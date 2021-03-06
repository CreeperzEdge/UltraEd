#include "Common.h"
#include "Gizmo.h"
#include "Debug.h"

CGizmo::CGizmo()
{
  m_modifierState = Translate;
  m_xAxisRot = D3DXVECTOR3(0, -D3DX_PI / 2, 0);
  m_yAxisRot = D3DXVECTOR3(D3DX_PI / 2, 0, 0);
  m_zAxisRot = D3DXVECTOR3(0, D3DX_PI, 0);
  m_worldSpaceToggled = TRUE;
  snapToGridToggled = false;
  
  SetupMaterials();
  SetupTransHandles();
  SetupScaleHandles();
  SetupRotateHandles();
  Reset();
}

CGizmo::~CGizmo()
{
}

void CGizmo::SetModifier(enum GizmoModifierState state)
{
  m_modifierState = state;
}

D3DXVECTOR3 CGizmo::GetModifyVector()
{
  switch(m_state)
  {
  case XAxis:
    return D3DXVECTOR3(1, 0, 0);
  case YAxis:
    return D3DXVECTOR3(0, 1, 0);
  case ZAxis:
    return D3DXVECTOR3(0, 0, 1);
  }
  
  return D3DXVECTOR3(0, 0, 0);
}

void CGizmo::Render(IDirect3DDevice8 *device, ID3DXMatrixStack *stack, CCamera *camera)
{
  // Scale the size of the gizmo based on the camera distance.
  D3DXVECTOR3 distance = m_gameObjects[0].GetPosition() - camera->GetPosition();
  float scaleFactor = camera->GetView() == CameraView::Perspective ? 0.2f : 0.1f;
  float length = D3DXVec3Length(&distance) * scaleFactor;
  SetScale(D3DXVECTOR3(length, length, length));

  // Render all gizmo handles.
  device->SetMaterial(&m_redMaterial);
  m_gameObjects[m_modifierState * 3 + 0].Render(device, stack);

  device->SetMaterial(&m_greenMaterial);
  m_gameObjects[m_modifierState * 3 + 1].Render(device, stack);

  device->SetMaterial(&m_blueMaterial);
  m_gameObjects[m_modifierState * 3 + 2].Render(device, stack);
}

void CGizmo::Release()
{
  for(int i = 0; i < 9; i++)
  {
    m_gameObjects[i].Release(GameObjectRelease::AllResources);
  }
}

bool CGizmo::Select(D3DXVECTOR3 orig, D3DXVECTOR3 dir)
{
  float dist = 0;

  if(m_gameObjects[m_modifierState * 3 + 0].Pick(orig, dir, &dist))
  {
    m_state = XAxis;
    return true;
  }
  else if(m_gameObjects[m_modifierState * 3 + 1].Pick(orig, dir, &dist))
  {
    m_state = YAxis;
    return true;
  }
  else if(m_gameObjects[m_modifierState * 3 + 2].Pick(orig, dir, &dist))
  {
    m_state = ZAxis;
    return true;
  }

  return false;
}

void CGizmo::SetPosition(D3DXVECTOR3 position)
{
  for(int i = 0; i < 9; i++)
  {
    m_gameObjects[i].SetPosition(position);
  }
}

void CGizmo::SetScale(D3DXVECTOR3 scale)
{
  // Can't scale lower than one.
  scale.x = scale.x < 1 ? 1 : scale.x;
  scale.y = scale.y < 1 ? 1 : scale.y;
  scale.z = scale.z < 1 ? 1 : scale.z;

  for(int i = 0; i < 9; i++)
  {
    m_gameObjects[i].SetScale(scale);
  }
}

void CGizmo::SetupMaterials()
{
  // Create the red material.
  ZeroMemory(&m_redMaterial, sizeof(D3DMATERIAL8));
  m_redMaterial.Emissive.r = 0.8f;
  m_redMaterial.Emissive.g = 0;
  m_redMaterial.Emissive.b = 0;
  
  // Create the green material.
  ZeroMemory(&m_greenMaterial, sizeof(D3DMATERIAL8));
  m_greenMaterial.Emissive.r = 0;
  m_greenMaterial.Emissive.g = 0.8f;
  m_greenMaterial.Emissive.b = 0;
  
  // Create the blue material.
  ZeroMemory(&m_blueMaterial, sizeof(D3DMATERIAL8));
  m_blueMaterial.Emissive.r = 0;
  m_blueMaterial.Emissive.g = 0;
  m_blueMaterial.Emissive.b = 0.8f;
}

void CGizmo::SetupTransHandles()
{
  m_gameObjects[0] = CGameObject("Assets/trans-gizmo.dae", GameObjectType::Model);
  m_gameObjects[0].Rotate(m_xAxisRot.y, D3DXVECTOR3(0, 1, 0));
  m_gameObjects[1] = CGameObject("Assets/trans-gizmo.dae", GameObjectType::Model);
  m_gameObjects[1].Rotate(m_yAxisRot.x, D3DXVECTOR3(1, 0, 0));
  m_gameObjects[2] = CGameObject("Assets/trans-gizmo.dae", GameObjectType::Model);
  m_gameObjects[2].Rotate(m_zAxisRot.y, D3DXVECTOR3(0, 1, 0));
}

void CGizmo::SetupScaleHandles()
{
  m_gameObjects[3] = CGameObject("Assets/scale-gizmo.dae", GameObjectType::Model);
  m_gameObjects[3].Rotate(m_xAxisRot.y, D3DXVECTOR3(0, 1, 0));
  m_gameObjects[4] = CGameObject("Assets/scale-gizmo.dae", GameObjectType::Model);
  m_gameObjects[4].Rotate(m_yAxisRot.x, D3DXVECTOR3(1, 0, 0));
  m_gameObjects[5] = CGameObject("Assets/scale-gizmo.dae", GameObjectType::Model);
  m_gameObjects[5].Rotate(m_zAxisRot.y, D3DXVECTOR3(0, 1, 0));
}

void CGizmo::SetupRotateHandles()
{
  m_gameObjects[6] = CGameObject("Assets/rot-gizmo.dae", GameObjectType::Model);
  m_gameObjects[6].Rotate(m_xAxisRot.y, D3DXVECTOR3(0, 1, 0));
  m_gameObjects[7] = CGameObject("Assets/rot-gizmo.dae", GameObjectType::Model);
  m_gameObjects[7].Rotate(m_yAxisRot.x, D3DXVECTOR3(1, 0, 0));
  m_gameObjects[8] = CGameObject("Assets/rot-gizmo.dae", GameObjectType::Model);
  m_gameObjects[8].Rotate(m_zAxisRot.y, D3DXVECTOR3(0, 1, 0));
}

void CGizmo::Update(CCamera *camera, D3DXVECTOR3 orig, D3DXVECTOR3 dir, CGameObject *currentGameObject, CGameObject *selectedGameObject)
{
  D3DXVECTOR3 targetDir = D3DXVECTOR3(0, 0, 0);
  D3DXVECTOR3 v0, v1, v2, intersectPoint;
  D3DXVECTOR3 look = selectedGameObject->GetPosition() - camera->GetPosition();
  D3DXVec3Normalize(&look, &look);

  // Determine orientation fo plane to produce depending on selected axis.
  if(m_state == XAxis)
  {
    D3DXVECTOR3 right = m_worldSpaceToggled ? D3DXVECTOR3(1, 0, 0) : selectedGameObject->GetRight();
    D3DXVECTOR3 up;
    D3DXVec3Cross(&up, &right, &look);
    D3DXVec3Cross(&look, &right, &up);

    v0 = selectedGameObject->GetPosition();
    v1 = v0 + right;
    v2 = v0 + up;

    targetDir = right;
  }
  else if(m_state == YAxis)
  {
    D3DXVECTOR3 up =  m_worldSpaceToggled ? D3DXVECTOR3(0, 1, 0) : selectedGameObject->GetUp();
    D3DXVECTOR3 right;
    D3DXVec3Cross(&right, &up, &look);
    D3DXVec3Cross(&look, &up, &right);

    v0 = selectedGameObject->GetPosition();
    v1 = v0 + right;
    v2 = v0 + up;

    targetDir = up;
  }
  else if(m_state == ZAxis)
  {
    D3DXVECTOR3 forward =  m_worldSpaceToggled ? D3DXVECTOR3(0, 0, 1) : selectedGameObject->GetForward();
    D3DXVECTOR3 up;
    D3DXVec3Cross(&up, &forward, &look);
    D3DXVec3Cross(&look, &forward, &up);

    v0 = selectedGameObject->GetPosition();
    v1 = v0 + forward;
    v2 = v0 + up;

    targetDir = forward;
  }

  D3DXPLANE testPlane;
  D3DXPlaneFromPoints(&testPlane, &v0, &v1, &v2);
  D3DXVECTOR3 rayEnd = orig + (dir * 1000);
  
  if(D3DXPlaneIntersectLine(&intersectPoint, &testPlane, &orig, &rayEnd) != NULL)
  {
    if(m_updateStartPoint == D3DXVECTOR3(-999, -999, -999))
    {
      m_updateStartPoint = intersectPoint;
    }

    D3DXVECTOR3 mouseDir = intersectPoint - m_updateStartPoint;
    D3DXVECTOR3 normMouseDir;
    D3DXVec3Normalize(&normMouseDir, &mouseDir);
    FLOAT moveDist = D3DXVec3Length(&mouseDir);
    const float snapSize = 0.5f;
    const float epsilon = 0.1f;
    bool shouldSnap = fabs((moveDist > snapSize ? snapSize : moveDist) - snapSize) < epsilon;

    // Clamp the dot product between -1, 1 to not cause a undefined result.
    FLOAT dot = D3DXVec3Dot(&targetDir, &normMouseDir);
    dot = dot < -1.0f ? -1.0f : dot > 1.0f ? 1.0f : dot;
    FLOAT angle = acosf(dot);

    // Only allow movement when mouse following axis.
    FLOAT modifier = 1.0f - (angle/(D3DX_PI/2));
    int sign = modifier < 0 ? -1 : 1;
    
    if(m_modifierState == Translate)
    {
      if(shouldSnap && snapToGridToggled)
      {
        
        D3DXVECTOR3 newPos = currentGameObject->GetPosition();
        newPos.x = round(newPos.x * (1 / snapSize)) / (1 / snapSize);
        newPos.y = round(newPos.y * (1 / snapSize)) / (1 / snapSize);
        newPos.z = round(newPos.z * (1 / snapSize)) / (1 / snapSize);
        currentGameObject->SetPosition(newPos + (targetDir * snapSize * sign));
      }
      else if(!snapToGridToggled)
      {
        currentGameObject->Move(targetDir * (moveDist * modifier));
      }
    }
    else if(m_modifierState == Scale)
    {
      currentGameObject->Scale(targetDir * (moveDist * modifier));
    }
    else
    {
      currentGameObject->Rotate(moveDist * modifier, targetDir);
    
      if(!m_worldSpaceToggled)
      {
        // Keep gizmo in-sync with the game object's rotation.
        for(int i = 0; i < 3; i++)
        {
          m_gameObjects[i * 3 + 0].SetLocalRotationMatrix(currentGameObject->GetRotationMatrix());
          m_gameObjects[i * 3 + 1].SetLocalRotationMatrix(currentGameObject->GetRotationMatrix());
          m_gameObjects[i * 3 + 2].SetLocalRotationMatrix(currentGameObject->GetRotationMatrix());
        }
      }
    }
    
    if(selectedGameObject == currentGameObject) 
    {
      if(shouldSnap || !snapToGridToggled) 
      {
        m_updateStartPoint = intersectPoint;
      }
    }
  }
  
  SetPosition(selectedGameObject->GetPosition());
}

void CGizmo::Reset()
{
  m_updateStartPoint = D3DXVECTOR3(-999, -999, -999);
}

bool CGizmo::ToggleSpace(CGameObject *gameObject)
{
  m_worldSpaceToggled = !m_worldSpaceToggled;

  D3DXMATRIX identity;
  D3DXMatrixIdentity(&identity);
  D3DXMATRIX mat = m_worldSpaceToggled ? identity : gameObject->GetRotationMatrix();

  for(int i = 0; i < 3; i++)
  {
    m_gameObjects[i * 3 + 0].SetLocalRotationMatrix(mat);
    m_gameObjects[i * 3 + 1].SetLocalRotationMatrix(mat);
    m_gameObjects[i * 3 + 2].SetLocalRotationMatrix(mat);
  }

  return m_worldSpaceToggled;
}

bool CGizmo::ToggleSnapping()
{
  snapToGridToggled = !snapToGridToggled;
  return snapToGridToggled;
}