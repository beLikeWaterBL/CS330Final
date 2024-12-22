///////////////////////////////////////////////////////////////////////////////
// SceneManager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		// conditionals for different images
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

// Custom item meshes
void SceneManager::MakeTable() {
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f,0.0f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	SetShaderTexture("fabric");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cloth");
	m_basicMeshes->DrawPlaneMesh();
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 20.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 45.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.01f, -10.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// draw the mesh with transformation values
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.5f);
	m_basicMeshes->DrawPlaneMesh();
}
void SceneManager::MakeCup(float xPos, float yPos, float zPos) {

	// declare variables for base transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	// create ref position variables relative to base object
	float refX = xPos;
	float refY = yPos;
	float refZ = zPos;
	// torus meshes for cup roundness

	scaleXYZ = glm::vec3(1.2f, 1.2f, 4.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos, zPos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	
	SetShaderTexture("ceramic_w");
	SetTextureUVScale(0.7, 0.7);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTorusMesh();

	// torus meshes for cup roundness

	scaleXYZ = glm::vec3(1.4f, 1.4f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 270.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos - 0.5f, zPos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("ceramic_w");
	SetTextureUVScale(1.0, 1.0);
	//SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTorusMesh();

	//tapered cylinder to give cleaner siding to cup

	scaleXYZ = glm::vec3(1.73f, 1.5f, 1.73f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos - 0.7f, zPos);

	// set the transformation to apply matrix transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("drywall");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// torus for cup bottom rim
	scaleXYZ = glm::vec3(.90f, .90f, 1.90f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 270.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos + 0.6f, zPos);

	// set the transformation to apply matrix transformation
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("drywall");
	SetTextureUVScale(1.0, 1.0);
	//SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTorusMesh();

}
void SceneManager::MakePlate(float xPos, float yPos, float zPos) {
	
	// declare variables for transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	// basic cylinder for the PLATE BOTTOM

	scaleXYZ = glm::vec3(1.5f, 0.4f, 1.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos, zPos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("overlay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawCylinderMesh();
	
	// tapered cylinder for the PLATE center of object

	
	//3.0f, 0.5f, 3.0f
	scaleXYZ = glm::vec3(3.5f, 1.0f, 3.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos + 1.4f, zPos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.95, .95, .95, 1);
	SetShaderTexture("overlay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTaperedCylinderMesh(true, false, true);
}
void SceneManager::MakeSaucer(float xPos, float yPos, float zPos) {

	// declare variables for transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	glm::vec3 localScale = glm::vec3(0.8);

	// basic cylinder for the PLATE BOTTOM

	scaleXYZ = glm::vec3(1.5f, 0.8f, 1.5f) * localScale;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos, zPos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("overlay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	// tapered cylinder for the PLATE center of object


	//3.0f, 0.5f, 3.0f
	scaleXYZ = glm::vec3(3.5f, 0.5f, 3.5f) * localScale;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos + 1.0f, zPos);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(.95, .95, .95, 1);
	SetShaderTexture("overlay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTaperedCylinderMesh(true, false, true);
}
void SceneManager::MakeSpoon(float xPos, float yPos, float zPos) {
	// declare variables for transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	// object scale
	glm::vec3 localScale = glm::vec3(0.5f);
	
	// spoon base with half sphere

	scaleXYZ = glm::vec3(2.0f, 0.5f, 1.0f) * localScale;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos , yPos , zPos);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("overlay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawHalfSphereMesh();
	
	scaleXYZ = glm::vec3(2.3f, 0.3f, 1.0f) * localScale;

	// set the XYZ rotation for the mesh
	XrotationDegrees = XrotationDegrees;
	YrotationDegrees = YrotationDegrees;
	ZrotationDegrees = ZrotationDegrees;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos, zPos);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("overlay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTaperedCylinderMesh(true, false, true);
}
void SceneManager::MakeBowl(float xPos, float yPos, float zPos) {
	// mesh objects for bowl
	// start from bottom, torus baseline object
	//tranformation variables
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	// full object scale
	glm::vec3 localScale = glm::vec3(1.0f);

	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.7f) * localScale;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos, zPos) * localScale;
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("overlay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawTorusMesh();

	// cylinder above the rounded torus bottom

	scaleXYZ = glm::vec3(1.1f, 0.2f, 1.1f) * localScale;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos + 0.1f, zPos) * localScale;
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("overlay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawCylinderMesh(false, false, true);


	// halfsphere for bowl above torus rim

	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f) * localScale;

	// set the XYZ rotation for the mesh
	XrotationDegrees = 180.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos + 1.9f, zPos) * localScale;
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("overlay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("ceramic");
	m_basicMeshes->DrawHalfSphereMesh();
}
void SceneManager::MakeChopSticksFront(float xPos, float yPos, float zPos) {
	// mesh objects for chopsticks
		// 2 box meshes
		//tranformation variables
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	// full object scale

	scaleXYZ = glm::vec3(0.2f, 0.2f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos, zPos);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("chopstick");
	SetTextureUVScale(1.0, 1.0);
	
	m_basicMeshes->DrawBoxMesh();

}
void SceneManager::MakeChopsticksBack(float xPos, float yPos, float zPos) {
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
	// full object scale
	

	scaleXYZ = glm::vec3(0.2f, 0.2f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 270.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(xPos, yPos, zPos);
	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("chopstick");
	SetTextureUVScale(1.0, 1.0);
	m_basicMeshes->DrawBoxMesh();
}

// load the scene textures
// 16 available textures to use
void SceneManager::LoadTextures() {
	// create boolean for return values
	bool tReturn = false;

	// create the textures using file path and make tag for texture
	// white ceramic mainly for the cup except the bottom rim
	tReturn = CreateGLTexture("Utilities/textures/ceramic_white.jpg", "ceramic_w");
	// overlay is a different flat color for the plate
	tReturn = CreateGLTexture("Utilities/textures/ceramic_overlay.png", "overlay");
	// fabric for the table cloth
	tReturn = CreateGLTexture("Utilities/textures/yellowTex.jpg", "fabric");
	// used drywall to emulate a grittiness for the bottom of the ceramic cup
	tReturn = CreateGLTexture("Utilities/textures/drywall.jpg", "ceramic_grit");
	// wood texture for the chopsticks
	tReturn = CreateGLTexture("Utilities/textures/woodTex.jpg", "chopstick");



	//bind the textures after they are created
	BindGLTextures();
}

// Object material creation for color+lighting
void SceneManager::DefineObjectMaterials() {
	
	OBJECT_MATERIAL ceramic_cup;
	// the natural light the object gives off
	ceramic_cup.ambientColor = glm::vec3(0.6, 0.6, 0.6);
	// value doesn't need to be high depending on the material
	ceramic_cup.ambientStrength = 0.4f;
	// color value of the light that is reflected off of the material at certain angles
	ceramic_cup.diffuseColor = glm::vec3(0.7, 0.7, 0.7);
	// light color based on light direction and view direction, similar to diffuse, intensity does not need to be high
	ceramic_cup.specularColor = glm::vec3(0.5, 0.5, 0.5);
	ceramic_cup.shininess = 80.0f;
	ceramic_cup.tag = "ceramic";
	m_objectMaterials.push_back(ceramic_cup);

	OBJECT_MATERIAL table_cloth;
	// the natural light the object gives off
	table_cloth.ambientColor = glm::vec3(0.1, 0.1, 0.1);
	// value doesn't need to be high depending on the material
	table_cloth.ambientStrength = 0.1f;
	// color value of the light that is reflected off of the material at certain angles
	table_cloth.diffuseColor = glm::vec3(0.2, 0.2, 0.2);
	// light color based on light direction and view direction, similar to diffuse, intensity does not need to be high
	table_cloth.specularColor = glm::vec3(0.0, 0.0, 0.0);
	table_cloth.shininess = 1.0f;
	table_cloth.tag = "cloth";
	m_objectMaterials.push_back(table_cloth);
}

// light def and creation
void SceneManager::LoadSceneLights() {
	
	// allow use of custom lighting
	m_pShaderManager->setBoolValue(g_UseLightingName, true);


	m_pShaderManager->setVec3Value("lightSources[0].position", 0.0f,6.0f,4.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.011f, 0.4f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.1f, 0.0f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.3f);


	m_pShaderManager->setVec3Value("lightSources[1].position", -6.0f, 1.0f, 4.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 0.01f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.9f);

}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the scene textures
	LoadTextures();
	DefineObjectMaterials();
	LoadSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// starting with center saucer and cup in the center of the picture, base coordinates made for this piece
	// every other object is positioned relative to the center piece
	float baseObX = -1.0f;
	float baseObY = 0.0f;
	float baseObZ = 0.0f;
	MakeTable();
	// center saucer + cup
	MakeSaucer(baseObX, baseObY, baseObZ);
	MakeCup(baseObX, baseObY + 1.55f, baseObZ);
	// bowl in front left of center piece
	MakeBowl(baseObX - 3.5f, baseObY + 0.1f, baseObZ + 3.6f);
	// plate front right of the center piece
	MakePlate(baseObX + 3.0f, baseObY, baseObZ + 4.6f);
	// chopstick set next to front plate
	MakeChopSticksFront(baseObX + 6.5f, baseObY + 0.1, baseObZ + 4.0f);
	MakeChopSticksFront(baseObX + 7.5f, baseObY + 0.1, baseObZ + 4.0f);
	// bowl behind center
	MakeBowl(baseObX + 6.5f, baseObY + 0.1f, baseObZ - 8.5f);
	// saucer + cup behind back bowl
	MakeSaucer(baseObX + 3.0f, baseObY, baseObZ - 11.0f);
	// furthest plate on table to right
	MakeCup(baseObX + 3.0f, baseObY + 1.55f, baseObZ - 11.0f);
	// plate for set in the far part of scene
	MakePlate(baseObX + 8.3f, baseObY, baseObZ - 13.6f);
	// chopsticks for the far plate set
	MakeChopsticksBack(baseObX + 5.0f, baseObY + 0.1, baseObZ - 16.0f);
	MakeChopsticksBack(baseObX + 5.0f, baseObY + 0.1f, baseObZ - 17.0f);

	

}
