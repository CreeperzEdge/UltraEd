#include "FileIO.h"
#include "cJSON.h"
#include <shlwapi.h>

bool CFileIO::Save(std::vector<CSavable*> savables)
{
  OPENFILENAME ofn;
  char szFile[260];
  
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "UltraEd (*.ultra)";
  ofn.nFilterIndex = 1;
  ofn.lpstrTitle = "Save Scene";
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_OVERWRITEPROMPT;
  
  if(GetSaveFileName(&ofn))
  {
    char *saveName = ofn.lpstrFile;

    // Add the extension if not supplied in the dialog.
    if(strstr(saveName, ".ultra") == NULL)
    {
      sprintf(saveName, "%s.ultra", saveName);
    }
    
    // Write the JSON data out.
    FILE *file = fopen(saveName, "w");
    std::vector<CSavable*>::iterator it;
    cJSON *root = cJSON_CreateObject();
    cJSON* array = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "models", array);

    for(it = savables.begin(); it != savables.end(); ++it)
    {
      Savable current = (*it)->Save();
      cJSON *object = current.object->child;

      if(current.type == SavableType::Editor)
      {
        cJSON_AddItemToObject(root, object->string, object);
      }
      else if(current.type == SavableType::Model)
      {
        cJSON_AddItemToArray(array, object);
      }
    }

    fprintf(file, cJSON_Print(root));
    cJSON_Delete(root);
    fclose(file);
  }
  
  return true;
}

bool CFileIO::Load(char** data)
{
  OPENFILENAME ofn;
  char szFile[260];
  
  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.lpstrFile[0] = '\0';
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "UltraEd (*.ultra)";
  ofn.nFilterIndex = 1;
  ofn.lpstrTitle = "Load Scene";
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
  
  if(GetOpenFileName(&ofn))
  {
    FILE *file = fopen(ofn.lpstrFile, "r");
    if(file == NULL) return false;

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char *contents = (char*)malloc(size);
    if(contents == NULL) return false;

    fread(contents, size, 1, file);
    fclose(file);

    *data = contents;

    return true;
  }

  return false;
}

FileInfo CFileIO::Import(const char* file)
{
  const char* root = "Library";
  const char* assets = "Assets/";

  if(CreateDirectory(root, NULL) || GetLastError() == ERROR_ALREADY_EXISTS)
  {
    char* name = PathFindFileName(file);
    char* target = (char*)malloc(128);
    char guidBuffer[40];
    GUID uniqueIdentifier;
    wchar_t guidWide[40];

    // Cache the starting directory.
    if(strlen(startingDir) == 0)
    {
      GetCurrentDirectory(128, startingDir);
    }

    if(strncmp(file, assets, strlen(assets)) == 0)
    {
      FileInfo info = { strdup(file), Editor };
      return info;
    }

    // Create a unique identifier and convert to a char array.
    CoCreateGuid(&uniqueIdentifier);
    StringFromGUID2(uniqueIdentifier, guidWide, 40);
    wcstombs(guidBuffer, guidWide, 40);

    // Remove the first and last curly brace from GUID.
    memmove(guidBuffer, guidBuffer + 1, strlen(guidBuffer));
    guidBuffer[strlen(guidBuffer) - 1] = '\0';

    // Format new imported path.
    sprintf(target, "%s\\%s\\%s-%s", startingDir, root, guidBuffer, name);

    if(CopyFile(file, target, FALSE))
    {
      FileInfo info = { target, User };
      return info;
    }
  }

  FileInfo info = { strdup(file), Unknown };
  return info;
}