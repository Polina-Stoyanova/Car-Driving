
#include <GL\glew.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <gl\GLU.h>

#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\type_ptr.hpp>

#include <iostream>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"

#include "GeometryNode.h"
#include "GroupNode.h"
#include "TransformNode.h"

#include <random>

#define playersNum 4

bool init();
bool initGL();
void render();

int OnCollisionEnter(GeometryNode* carPlayers, TransformNode* transformPlayers);
void MoveForward(GeometryNode* carPlayers, TransformNode* transformPlayers);
void GameOver();

void close();

void CreateScene();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;
Shader gShader;

TransformNode* trRoad;
TransformNode* trCar1;
TransformNode* trCar2; //player
TransformNode* trCar3;
TransformNode* trCar4;
TransformNode* trCar5;
TransformNode* trGO;

GeometryNode* road;
GeometryNode* car1;
GeometryNode* car2; //player
GeometryNode* car3;
GeometryNode* car4;
GeometryNode* car5;
GeometryNode* gameOver;

TransformNode* transforms[playersNum];
GeometryNode* players[playersNum];

int endRoad = 0;
int collCount = 0;
int flag = 0;
int collFlag = 0;
int sceneCreated = 0;
float speed = 0.007f;
float carsSpeed = 0.009f;
float moveSpeed = 0.2f; //left-right

GroupNode* gRoot;

GLuint gVAO, gVBO;
GLuint texID1;

// camera
Camera camera(glm::vec3(-0.5f, 7.0f, 15.0f)); 
float lastX = -1;
float lastY = -1;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// lighting
glm::vec3 lightPos(1.2f, 1.0f, 15.0f);

//statics
unsigned int Node::genID;
glm::mat4 TransformNode::transformMatrix = glm::mat4(1.0f);

GeometryNode* gTestNode;
TransformNode* selectedNode;


//event handlers
void HandleKeyDown(const SDL_KeyboardEvent& key);
void HandleMouseMotion(const SDL_MouseMotionEvent& motion);
void HandleMouseWheel(const SDL_MouseWheelEvent& wheel);
void HandleMouseButtonUp(const SDL_MouseButtonEvent& button);

//raycasting
void ScreenPosToWorldRay(
	int mouseX, int mouseY,             // Mouse position, in pixels, in window coordinates
	int viewportWidth, int viewportHeight,  // Viewport size, in pixels
	glm::mat4 ViewMatrix,               // Camera position and orientation
	glm::mat4 ProjectionMatrix,         // Camera parameters (ratio, field of view, near and far planes)
	glm::vec3& out_direction            // Ouput : Direction, in world space, of the ray that goes "through" the mouse.
);



int main(int argc, char* args[])
{
	init();
	CreateScene();

	SDL_Event e;
	//While application is running
	bool quit = false;
	while (!quit)
	{
		// per-frame time logic
		float currentFrame = SDL_GetTicks() / 1000.0f;
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					quit = true;
				}
				else
				{
					HandleKeyDown(e.key);
				}
				break;
			}
		}

		if (flag == 0 && camera.Position.z > -15.0f) {
			camera.Position.z -= speed; 
			lightPos.z -= speed; 
			trCar2->Move(glm::vec3(0.0f, 0.0f, -speed)); 
			
			trCar1->Move(glm::vec3(0.0f, 0.0f, carsSpeed)); 
			trCar3->Move(glm::vec3(0.0f, 0.0f, carsSpeed)); 
			trCar4->Move(glm::vec3(0.0f, 0.0f, carsSpeed)); 
			trCar5->Move(glm::vec3(0.0f, 0.0f, carsSpeed)); 
		}
		for (int i = 0; i < playersNum; i++)
		{
			MoveForward(players[i], transforms[i]);
		}

		for (int i = 0; i < playersNum; i++)
		{
			collFlag = OnCollisionEnter(players[i], transforms[i]);

			if (collFlag == 1)
			{
				flag = 1;

				collCount++;

				if (collCount == 1)
				{
					camera.ChangeCameraPos(glm::vec3(camera.Position.x, camera.Position.y + 2.0f, camera.Position.z));
				}
				GameOver();	
			}
		}

		if (camera.Position.z <= -15.0f)
		{	
			endRoad++;

			if(endRoad == 1) camera.ChangeCameraPos(glm::vec3(camera.Position.x, camera.Position.y + 2.0f, camera.Position.z));
			GameOver();
		}

		//Render
		render();

		//Update screen
		SDL_GL_SwapWindow(gWindow);
	}
	 
	close();
	return 0;
}


void HandleKeyDown(const SDL_KeyboardEvent& key)
{
	switch (key.keysym.sym)
	{
	case SDLK_LEFT:
		if (trCar2->GetPosition().x > trRoad->GetPosition().x - 2.3f) 
		{
			trCar2->Move(glm::vec3(-moveSpeed, 0.0f, 0.0f)); 
		}
		break;

	case SDLK_RIGHT:
		if (trCar2->GetPosition().x < trRoad->GetPosition().x + 1.0f)
		{
			trCar2->Move(glm::vec3(moveSpeed, 0.0f, 0.0f)); 
		}
		break;
	}

}


bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Use OpenGL 3.3
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);


		//Create window
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN /*| SDL_WINDOW_FULLSCREEN*/);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			//Create context
			gContext = SDL_GL_CreateContext(gWindow);
			if (gContext == NULL)
			{
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				//Use Vsync
				if (SDL_GL_SetSwapInterval(1) < 0)
				{
					printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
				}

				//Initialize OpenGL
				if (!initGL())
				{
					printf("Unable to initialize OpenGL!\n");
					success = false;
				}
			}
		}
	}

	return success;
}



bool initGL()
{
	bool success = true;
	GLenum error = GL_NO_ERROR;

	glewInit();

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		success = false;
		printf("Error initializing OpenGL! %s\n", gluErrorString(error));
	}

	glClearColor(0.0f, 0.5f, 0.0f, 1.0f);

	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	gShader.Load("./shaders/vertex.vert", "./shaders/fragment.frag");
	
	return success;
}

void CreateScene()
{
	gRoot = new GroupNode("Root");

	//TransformNode* trRoad = new TransformNode("Road Transform");
	trRoad = new TransformNode("Road Transform");
	trRoad->SetTranslation(glm::vec3(0.0f, 0.0f, 0.0f));
	trRoad->SetRotation(90.0f); 
	trRoad->SetScale(glm::vec3(1.2f, 1.2f, 1.2f));
	//GeometryNode* road = new GeometryNode("Road");
	road = new GeometryNode("Road");
	road->LoadFromFile("models/Road/untitled.obj");
	road->SetShader(&gShader);
	//gTestNode = road;
	gRoot->AddChild(trRoad);
	trRoad->AddChild(road);

	//TransformNode* trCar1 = new TransformNode("Car1 Transform");
	trCar1 = new TransformNode("Car1 Transform");
	trCar1->SetTranslation(glm::vec3(0.5f, 2.0f, 10.0f)); //15z e kamerata 
	trCar1->SetRotation(0.0f); 
	trCar1->SetScale(glm::vec3(0.4f, 0.4f, 0.4f));
	//GeometryNode* car1 = new GeometryNode("Car1");
	car1 = new GeometryNode("Car1");
	car1->LoadFromFile("models/car models 2/Low Poly Cars Pack 1/OBJ/BasicCar.obj");//car1
	car1->SetShader(&gShader);
	trCar1->AddChild(car1);
	gRoot->AddChild(trCar1);


	//TransformNode* trCar2 = new TransformNode("Car2 Transform");
	trCar2 = new TransformNode("Car2 Transform");
	trCar2->SetTranslation(glm::vec3(-1.5f, 2.0f, 15.0f));
	trCar2->SetRotation(-180.0f); 
	trCar2->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));
	//trCar2->SetTranslation(glm::vec3(0.0f, 2.0f, 0.0f));
	//GeometryNode* car2 = new GeometryNode("Car2");
	car2 = new GeometryNode("Car2");
	car2->LoadFromFile("models/car models 2/Low Poly Cars Pack 1/OBJ/RaceCar.obj");// car2 - taxi
	car2->SetShader(&gShader);
	trCar2->AddChild(car2);
	gRoot->AddChild(trCar2);


	//TransformNode* trCar3 = new TransformNode("Car3 Transform");
	trCar3 = new TransformNode("Car3 Transform");
	trCar3->SetTranslation(glm::vec3(0.5f, 2.0f, -1.0f)); 
	trCar3->SetRotation(0.0f); 
	trCar3->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
	//GeometryNode* car3 = new GeometryNode("Car3");
	car3 = new GeometryNode("Car3");
	car3->LoadFromFile("models/car models 2/Low Poly Cars Pack 1/OBJ/Taxi.obj"); // car 3 - - bqlo, cherveno, cherno
	car3->SetShader(&gShader);
	//carModel3 = car3;
	trCar3->AddChild(car3);
	gRoot->AddChild(trCar3);


	//TransformNode* trCar4 = new TransformNode("Car4 Transform");
	trCar4 = new TransformNode("Car4 Transform");
	trCar4->SetTranslation(glm::vec3(-1.5f, 2.0f, 7.0f)); 
	trCar4->SetScale(glm::vec3(0.3f, 0.3f, 0.3f));
	trCar4->SetRotation(-90.0f); 
	//GeometryNode* car4 = new GeometryNode("Car4");
	car4 = new GeometryNode("Car4");
	car4->LoadFromFile("models/car models 2/Low Poly Cars Pack 2/OBJ/Truck.obj"); // Truck
	car4->SetShader(&gShader);
	//carModel4 = car4;
	trCar4->AddChild(car4);
	gRoot->AddChild(trCar4);

	
	//TransformNode* trCar4 = new TransformNode("Car4 Transform");
	trCar5 = new TransformNode("Car5 Transform");
	trCar5->SetTranslation(glm::vec3(-1.5f, 2.0f, 3.0f)); 
	trCar5->SetRotation(0.0f); 
	trCar5->SetScale(glm::vec3(0.5f, 0.5f, 0.5f));
	//GeometryNode* car5 = new GeometryNode("Car5");
	car5 = new GeometryNode("Car5");
	car5->LoadFromFile("models/car models 2/Low Poly Cars Pack 1/OBJ/BasicCar.obj"); // newCar
	car5->SetShader(&gShader);
	//carModel5 = car5;
	trCar5->AddChild(car5);
	gRoot->AddChild(trCar5);

	players[0] = car1;
	players[1] = car3;
	players[2] = car4;
	players[3] = car5;

	transforms[0] = trCar1;
	transforms[1] = trCar3;
	transforms[2] = trCar4;
	transforms[3] = trCar5;
}



void GameOver()
{
	//TransformNode* trGO = new TransformNode("GameOver Transform");
	trGO = new TransformNode("GameOver Transform");
	trGO->SetTranslation(glm::vec3(camera.Position.x - 1.0f, 2.0f, trCar2->GetPosition().z +0.5f));
	
	trGO->SetRotation(0.0f); 
	trGO->SetScale(glm::vec3(1.2f, 1.2f, 1.2f));
	//GeometryNode* gameOver = new GeometryNode("GameOver");
	gameOver = new GeometryNode("GameOver");
	gameOver->LoadFromFile("models/Road/Game Over/gameOver.obj");
	gameOver->SetShader(&gShader);
	gRoot->AddChild(trGO);
	trGO->AddChild(gameOver);
}


void close()
{
	//delete GL programs, buffers and objects
	glDeleteProgram(gShader.ID);
	glDeleteVertexArrays(1, &gVAO);
	glDeleteBuffers(1, &gVBO);

	//Delete OGL context
	SDL_GL_DeleteContext(gContext);
	//Destroy window	
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}




void render()
{
	//Clear color buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), 4.0f / 3.0f, 0.1f, 10.0f);
	
	glUseProgram(gShader.ID);
	gShader.setMat4("view", view);
	gShader.setMat4("proj", proj);
	gShader.setFloat("near", 0.01f);
	gShader.setFloat("far", 20.0f);
	
	//lighting
	gShader.setVec3("light.diffuse", 1.0f, 1.0f, 1.0f);
	gShader.setVec3("light.position", lightPos);
	gShader.setVec3("viewPos", camera.Position);

	gRoot->Traverse();
}


int OnCollisionEnter(GeometryNode* carPlayers, TransformNode* transformPlayers)
{
	float car2_radiusX = car2->GetBoundingBox().Get_worldRadiusX();
	float car2_radiusZ = car2->GetBoundingBox().Get_worldRadiusZ();

	float carPlayers_radiusX = carPlayers->GetBoundingBox().Get_worldRadiusX();
	float carPlayers_radiusZ = carPlayers->GetBoundingBox().Get_worldRadiusZ();

	glm::vec3 carPlayers_WorldCenter = carPlayers->GetBoundingBox().GetWorldCenter();
	glm::vec3 car2_WorldCenter = car2->GetBoundingBox().GetWorldCenter();

	
	if ( (trCar2->GetPosition().x >= transformPlayers->GetPosition().x - carPlayers_radiusX && 
		trCar2->GetPosition().x <= transformPlayers->GetPosition().x + carPlayers_radiusX)  &&
		(trCar2->GetPosition().z - car2_radiusZ <= transformPlayers->GetPosition().z + carPlayers_radiusZ  && 
		trCar2->GetPosition().z + car2_radiusZ  >= transformPlayers->GetPosition().z - carPlayers_radiusZ )) return 1;

	
	if ( (trCar2->GetPosition().z >= transformPlayers->GetPosition().z - carPlayers_radiusZ &&
		trCar2->GetPosition().z <= transformPlayers->GetPosition().z + carPlayers_radiusZ) &&
		(trCar2->GetPosition().x - car2_radiusX - 0.2f  <= transformPlayers->GetPosition().x + carPlayers_radiusX + 0.2f && 
		( (trCar2->GetPosition().x + car2_radiusX + 0.2f >= transformPlayers->GetPosition().x - carPlayers_radiusX - 0.2f) ||  
		(trCar2->GetPosition().x - car2_radiusX + 0.2f >= transformPlayers->GetPosition().x - carPlayers_radiusX - 0.2f)))) return 1;

	return 0;
}


void MoveForward(GeometryNode* carPlayers, TransformNode* transformPlayers)
{
	float distance = glm::sqrt(pow(trCar2->GetPosition().x - transformPlayers->GetPosition().x,2) + 
		pow(trCar2->GetPosition().z - transformPlayers->GetPosition().z,2));

	if (distance >= 4.0f && transformPlayers->GetPosition().z > trCar2->GetPosition().z)
	{
		int i;
		float farDistance = transforms[0]->GetPosition().z;

		for (i = 0; i < playersNum; i++)
		{
			if (farDistance > transforms[i]->GetPosition().z) 
			{
				farDistance = transforms[i]->GetPosition().z;
			}
		} 
		int t = rand();

		if (t % 2 == 0)
		{
			transformPlayers->SetTranslation(glm::vec3(-1.5f, 2.0f, farDistance - 4.0f)); 
		}
		else {
			transformPlayers->SetTranslation(glm::vec3(0.5f, 2.0f, farDistance - 4.0f)); 
		}	
	}
}




