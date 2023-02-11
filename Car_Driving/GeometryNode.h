#pragma once

#include "Node.h"
#include "TransformNode.h"
#include "Model.h"
#include "BoundingObjects.h"
#include <glm\gtc\matrix_transform.hpp>


class GeometryNode : public Node
{
	Model model;
	Shader* shader;
	//BoundingSphere* boundingSphere = NULL;
	aabb* boundingBox = NULL;

public:
	GeometryNode() :  Node()
	{
		type = nt_GeometryNode;
	}

	GeometryNode(const std::string& name) : Node(name, nt_GeometryNode)
	{

	}

	GeometryNode(const std::string& name, const std::string& path) : Node(name, nt_GeometryNode)
	{
		LoadFromFile(path);
	}

	~GeometryNode()
	{	/*
		if (boundingSphere != NULL)
		{
			delete boundingSphere;
		}*/
		if (boundingBox != NULL)
		{
			delete boundingBox;
		}

	}

	void LoadFromFile(const std::string& path)
	{
		model.LoadModel(path);
		//boundingSphere = new BoundingSphere(this, model);
		boundingBox = new aabb(this, model);
	}

	const Model& GetModel() const
	{
		return model;
	}

	void SetShader(Shader* s)
	{
		shader = s;
	}

	/*
	const BoundingSphere& GetBoundingSphere()
	{
		return *boundingSphere;
	}*/

	const aabb& GetBoundingBox()
	{
		return *boundingBox;
	}


	void Traverse()
	{
		glm::mat4 transform = TransformNode::GetTransformMatrix();
		shader->setMat4("model", transform);
		glm::mat3 normalMat = glm::transpose(glm::inverse(transform));
		shader->setMat3("normalMat", normalMat);
		//boundingSphere->Transform(transform);
		boundingBox->Transform(transform);
		model.Draw(*shader);
	}
	
};
