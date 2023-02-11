#pragma once

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include "GroupNode.h"

//#include "BoundingObjects.h"


class TransformNode : public GroupNode
{
	glm::vec3 translation;
	glm::vec3 rotation;
	glm::vec3 scale;
	float angle;

	static glm::mat4 transformMatrix;

public:
	TransformNode(const std::string& name) : GroupNode(name)
	{
		type = nt_TransformNode;
		translation = glm::vec3(0.0f);
		rotation = glm::vec3(0.0f);
		scale = glm::vec3(1.0f);
		angle = 0;
	}

	glm::vec3 GetPosition() 
	{
		return translation;
	}

	void Move(glm::vec3 relativeTr) { 
		translation += relativeTr;
	}

	void SetTranslation(const glm::vec3& tr)
	{
		translation = tr;
	}

	void SetScale(const glm::vec3& sc)
	{
		scale = sc;
	}

	glm::vec3 GetScale() {
		return scale;
	}

	void SetRotation(float angleRot)
	{
		angle = angleRot;
		rotation = glm::vec3(0.0f, 1.0f, 0.0f);
		//transformMatrix = glm::rotate(transformMatrix, glm::radians(angleRot), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	void Traverse()
	{
		//push
		glm::mat4 matCopy = transformMatrix;

		transformMatrix = glm::translate(transformMatrix, translation);
		transformMatrix = glm::scale(transformMatrix, scale);
		transformMatrix = glm::rotate(transformMatrix, glm::radians(angle), rotation);

		for (unsigned int i = 0; i < children.size(); i++)
		{
			children[i]->Traverse();
		}

		//pop
		transformMatrix = matCopy;
	}


	static const glm::mat4 GetTransformMatrix()
	{
		return transformMatrix;
	}
};