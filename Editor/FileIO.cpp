#include "FileIO.h"
#include "Util.h"
#include "cJSON.h"
#include "microtar.h"
#include "fastlz.h"
#include "Dialog.h"
#include <shlwapi.h>

bool CFileIO::Save(vector<CSavable*> savables, string &fileName)
{ 
  string file;

  if(CDialog::Save("Save Scene", APP_FILE_FILTER, file))
  {
    // Add the extension if not supplied in the dialog.
    if(file.find(APP_FILE_EXT) == string::npos) file.append(APP_FILE_EXT);

    fileName = CleanFileName(file.c_str());

    // Prepare tar file for writing.
    mtar_t tar;
    mtar_open(&tar, file.c_str(), "w");
    
    // Write the scene JSON data.
    cJSON *root = cJSON_CreateObject();

    cJSON *cameraArray = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "cameras", cameraArray);

    cJSON *gameObjectArray = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "gameObjects", gameObjectArray);

    for(vector<CSavable*>::iterator it = savables.begin(); it != savables.end(); ++it)
    {
      Savable current = (*it)->Save();
      cJSON *object = current.object->child;

      // Add array to hold all attached resources.
      cJSON *resourceArray = cJSON_CreateArray();
      cJSON_AddItemToObject(object, "resources", resourceArray);

      // Rewrite and archive the attached resources.
      map<string, string> resources = (*it)->GetResources();
      for(map<string, string>::iterator rit = resources.begin(); rit != resources.end(); ++rit)
      {
        const char *fileName = PathFindFileName(rit->second.c_str());
        FILE *file = fopen(rit->second.c_str(), "rb");

        if(file == NULL) continue;

        cJSON *item = cJSON_CreateObject();
        cJSON_AddStringToObject(item, rit->first.c_str(), fileName);
        cJSON_AddItemToArray(resourceArray, item);
        
        // Calculate resource length.
        fseek(file, 0, SEEK_END);
        long fileLength = ftell(file);
        rewind(file);

        // Read all contents of resource into a buffer.
        char *fileContents = (char*)malloc(fileLength);
        fread(fileContents, fileLength, 1, file);

        // Write the buffer to the tar archive.
        mtar_write_file_header(&tar, fileName, fileLength);
        mtar_write_data(&tar, fileContents, fileLength);

        fclose(file);
        free(fileContents);
      }

      if(current.type == SavableType::Camera)
      {
        cJSON_AddItemToArray(cameraArray, object);
      }
      else if(current.type == SavableType::GameObject)
      {
        cJSON_AddItemToArray(gameObjectArray, object);
      }
    }

    const char *rendered = cJSON_Print(root);
    cJSON_Delete(root);

    mtar_write_file_header(&tar, "scene.json", strlen(rendered));
    mtar_write_data(&tar, rendered, strlen(rendered));

    mtar_finalize(&tar);
    mtar_close(&tar);

    return Compress(file.c_str());
  }
  
  return false;
}

bool CFileIO::Load(cJSON **data, string &fileName)
{
  string file;
  
  if(CDialog::Open("Load Scene", APP_FILE_FILTER, file) && Decompress(file))
  {
    fileName = CleanFileName(file.c_str());

    mtar_t tar;
    mtar_header_t header;

    mtar_open(&tar, file.c_str(), "r");
    mtar_find(&tar, "scene.json", &header);

    char *contents = (char*)calloc(1, header.size + 1);
    mtar_read_data(&tar, contents, header.size);
    cJSON *root = cJSON_Parse(contents);

    // Iterate through all game objects.
    cJSON *gameObjects = cJSON_GetObjectItem(root, "gameObjects");
    cJSON *gameObject = NULL;
    cJSON_ArrayForEach(gameObject, gameObjects)
    {
      // Locate each packed game object resource.
      cJSON *resources = cJSON_GetObjectItem(gameObject, "resources");
      cJSON *resource = NULL;
      cJSON_ArrayForEach(resource, resources)
      {
        char target[128];
        const char *fileName = resource->child->valuestring;

        // Locate the resource to extract.
        mtar_find(&tar, fileName, &header);
        char *buffer = (char*)calloc(1, header.size + 1);
        mtar_read_data(&tar, buffer, header.size);

        // Format path and write to library.
        string rootPath = CUtil::RootPath();
        sprintf(target, "%s\\%s", rootPath.c_str(), fileName);
        FILE *file = fopen(target, "wb");
        fwrite(buffer, 1, header.size, file);
        fclose(file);
        free(buffer);

        // Update the path to the fully qualified target.
        resource->child->valuestring = strdup(target);
      }
    }

    mtar_close(&tar);
    free(contents);
    remove(file.c_str());

    // Pass the constructed json object out.
    *data = root;

    return true;
  }

  return false;
}

FileInfo CFileIO::Import(const char *file)
{
  FileInfo info;
  string rootPath = CUtil::RootPath();

  // When a GUID then must have already been imported so don't re-import.
  if(CUtil::StringToGuid(PathFindFileName(file)) != GUID_NULL)
  {
    info.path = file;
    info.type = User;
    return info;
  }

  if(CreateDirectory(rootPath.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
  {
    char target[MAX_PATH];

    const char *assets = "Assets/";
    if(strncmp(file, assets, strlen(assets)) == 0)
    {
      info.path = file;
      info.type = Editor;
      return info;
    }

    // Format new imported path.
    sprintf(target, "%s\\%s", rootPath.c_str(), CUtil::GuidToString(CUtil::NewGuid()).c_str());

    if(CopyFile(file, target, FALSE))
    {
      info.path = target;
      info.type = User;
      return info;
    }
  }

  info.path = file;
  info.type = Unknown;
  return info;
}

bool CFileIO::Compress(string path)
{
  FILE *file = fopen(path.c_str(), "rb");
  if(file == NULL) return false;

  // Get the total size of the file.
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  // Read in entire file.
  char *data = (char*)malloc(size);
  if(data == NULL) return false;
  int bytesRead = fread(data, 1, size, file);
  if(bytesRead != size) return false;
  fclose(file);

  // Compressed buffer must be at least 5% larger.
  char *compressed = (char*)malloc(size + (size * 0.05));
  if(compressed == NULL) return false;
  int bytesCompressed = fastlz_compress(data, size, compressed);
  if(bytesCompressed == 0) return false;

  // Annotate compressed data with uncompressed size.
  int annotatedSize = bytesCompressed + sizeof(int);
  char *buffer = (char*)malloc(annotatedSize);
  memcpy(buffer, &size, sizeof(int));
  memcpy(buffer + sizeof(int), compressed, bytesCompressed);

  // Write compressed file back out.
  file = fopen(path.c_str(), "wb");
  if(file == NULL) return false;
  unsigned int bytesWritten = fwrite(buffer, 1, annotatedSize, file);
  if(bytesWritten != annotatedSize) return false;
  fclose(file);

  free(compressed);
  free(buffer);
  free(data);

  return true;
}

bool CFileIO::Decompress(string &path)
{
  FILE *file = fopen(path.c_str(), "rb");
  if(file == NULL) return false;

  // Get the total size of the file.
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  rewind(file);

  // Read in entire file.
  char *data = (char*)malloc(size);
  if(data == NULL) return false;
  int bytesRead = fread(data, 1, size, file);
  if(bytesRead != size) return false;
  fclose(file);

  // Read the uncompressed file length.
  int uncompressedSize = 0;
  memmove(&uncompressedSize, data, sizeof(int));
  memmove(data, data + sizeof(int), size - sizeof(int));

  char *decompressed = (char*)malloc(uncompressedSize);
  if(decompressed == NULL) return false;
  int bytesDecompressed = fastlz_decompress(data, size - sizeof(int), decompressed, uncompressedSize);
  if(bytesDecompressed == 0) return false;

  // Create a temp path to extract the scene file.
  path.append(tmpnam(NULL));
  string::size_type pos = path.find_last_of("\\");
  if(pos != string::npos) path.erase(pos, 1);

  // Write decompressed file back out.
  file = fopen(path.c_str(), "wb");
  if(file == NULL) return false;
  unsigned int bytesWritten = fwrite(decompressed, 1, bytesDecompressed, file);
  if(bytesWritten != bytesDecompressed) return false;
  fclose(file);
  free(decompressed);
  free(data);

  return true;
}

string CFileIO::CleanFileName(const char *fileName)
{
  string cleanedName(PathFindFileName(fileName));
  string::size_type pos = cleanedName.find('.');
  if(pos != string::npos) cleanedName.erase(pos, string::npos);
  return cleanedName;
}
