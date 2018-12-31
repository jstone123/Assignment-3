// Assignment 3.cpp: A program using the TL-Engine
//JStone3 G20654574.

#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <string>
#include <sstream>
#include <iostream>
#include <math.h>
using namespace tle;

//Constants used to hold numbers of each game object, used for creating arrays of models and collision detection etc.
const int numCheckPoints = 7;
const int numCheckpointStates = 7;
const int numWaypoints = 18;
const int numTanks = 16;
const int numIsles = 18;
const int numWalls = 12;

const float checkpointWidth = 9.8f;//This value is used to offset the hitbox when checking for collision between the car and the struts of the checkpoints.

//Rotation values for checkpoints that are rotated to fit into the course.
const float checkpoint2Rotation = 60.f;
const float checkpoint3Rotation = 130.f;
const float checkpoint5Rotation = 10.f;
const float checkpoint6Rotation = 300.f;

//Numbers for each checkpoints.
const int checkpointNum0 = 0;
const int checkpointNum1 = 1;
const int checkpointNum2 = 2;
const int checkpointNum3 = 3;
const int checkpointNum4 = 4;
const int checkpointNum5 = 5;
const int checkpointNum6 = 6;
const int checkpointNum7 = 7;
//Waypoint Numbers at which enemy car changes speed.
const int waypoint4 = 4;
const int waypoint8 = 8;
const int waypoint12 = 12;

const float crossCheckpointYOffest = 10.f;//Height of checkpoint model, used to determine where to place a cross to indicate a successful stage completion.

//Camera Values used to switch between first and third person camera.
const float fpCameraY = 3.f;
const float fpCameraZ = 6.f;
const float normalCameraY = 10.f;
const float normalCameraZ = -50.f;

const float crossScaleFactor = 0.5f;//Scale value used to reduce size of cross Model.

//Forward decleration of all functions.

void FirstPersonCamera(ICamera* camera, IModel* car);//Function used to switch to first person camera.

void ResetCamera(ICamera* camera, IModel* car, float rotation);//Function used to reset the camera back to third person. 

float CollisionDetection(IModel* car, IModel* checpoint);//Function used to detect whether the player car has passed through a checkpoint.

bool sphereBoxCollisionDetection(IModel* sphere, IModel* box, float sphereRad, float boxWidth, float boxLength);//Function used to check for collision between car and walls.
//Functions used to check for collision between car and left/right strut of checkpoints.
float CollisionDetectionCheckpointRightStrut(IModel* stationaryObject, IModel* movingObject, int checkpointNumber);
float CollisionDetectionCheckpointLeftStrut(IModel* stationaryObject, IModel* movingObject, int checkpointNumber);

void DisplayCross(IModel* checkpoint, IModel* &cross, IMesh* crossMesh, int checkpointNumber);//Function used to display a cross to indicate a successful stage completion.


//Structure holding variables used by cars.
struct cars
{

	IModel* car;//Car Model.
	int health = 100;//Health variable used when car collides with walls/struts.
	float raceTimer = 0;//Timer used to record how long it takes for the car to reach the end of the track.
	bool raceFinished = false;//Bool used to stop the timer when the car has finished the race.

};

void main()
{
	// Create a 3D engine (using TLX engine here) and open a window for it
	I3DEngine* myEngine = New3DEngine( kTLX );
	myEngine->StartWindowed();

	// Add default folder for meshes and other media
	myEngine->AddMediaFolder( ".\\Media" );

	/**** Set up your scene here ****/
	//Fonts used to display text on screen to the user.
	IFont* HUDFont = myEngine->LoadFont("Arial", 24);	
	IFont* myFont = myEngine->LoadFont("Arial", 36);

	//Enum to record what state the game is in.
	enum gameStates {start, racing, raceEnd};
	gameStates overallGameState = start;
	//Enum to record what checkpoints have been passed and this is used to prevent the course being completed out of order.
	enum checkpointStates { state1, state2, state3, state4, state5, state6, state7, stateOver };
	checkpointStates checkpointState = state1;

	//Bool used to indicate if a collision has occured with a wall.
	bool sphereBoxCollision;

	//Enum to record what text to show. This is used to provide a 3,2,1 countdown to the user.
	//And also prevent the car from moving until the countdown has ended.
	enum textStates {Three,Two,One,Go,NoTextAfter, NoTextBefore};
	textStates textState = NoTextBefore;
	//Strings used to hold model names.
	const string checkpointModel = "Checkpoint.x";
	const string islesModel = "IsleStraight.x";
	const string wallModel = "Wall.x";
	const string skyboxModel = "Skybox 07.x";
	const string carModel = "race2.x";
	const string crossModel = "Cross.x";
	const string groundModel = "ground.x";
	const string enemyCarSkin = "td_interstellar.jpg";
	const string carDamageSkin1 = "sp02_02.jpg";
	const string carDamageSkin2 = "sp02-01.jpg";
	const string carDamageSkin3 = "sp01.jpg";
	const string dummyModel = "Dummy.x";
	const string tank1Model = "TankSmall1.X";
	const string tank2Model = "TankSmall2.X";
	const string backdropModel = "backdrop.jpg";
	//Strings used to hold basic information that will be displayed to the user.
	const string boostActiveString = "Boost active";
	const string startString = "Hit Space to Start.";
	const string goString = "Go!";
	const string ready = "Ready;";
	const string stage1 = "Stage 1";
	const string raceOver = "Race complete";
	const string boostOverheatString = "Boost overheat in 1 second";
	const string gameOverEndString = "Game Over, press Esc to Exit";
	const string victory = "You won the race";
	const string defeat = "You lost the race";
	const string carDestroyed = "Your car was destroyed, you lost";
	
	//Values used in collision detection, these are the sizes of the collision areas.
	const float carRadiusAddCheckpointRadius = 7.f;
	const float carRadiusAddStrutRadius = 2.f;
	const float carRadiusAddTankRadius = 6.0f;
	
	bool drawText = true;//Bool used to control when the countdown should be drawn on the screen.
	float textCounter = 0;//Counter used to count and limit how long a piece of text is displayed for.
	float boostCounter = 0;//Counter used to count and limit how long the car speed boost is active for.
	//Variables used in controlling the car speed boost.
	const float boostDuration = 3.f;
	const float boostSpeedChange = 0.1f;//Value at which the car accelerates during the boost, it also decelerates at this speed as well, rapidly over 1s.
	//The boost counter starts at 0s, over the first 3 seconds the car accelerates up to the max speed of 120. 
	const float boostAccelerationStartWarning = 2.f;//At 2 seconds, a warning is displayed indicating the boost is about to overheat.
	const float boostAccelerationStopWarning = 3.f;//This warning is displayed for 1 second. 3-4s is the deceleration period.
	const float boostDecelerationPeriod = 4.f;//The boost then stops and the car rapidly decelerates over 1s.
	const float boostPeriodOver = 8.f;//The boost is then on cooldown for 5 seconds.
	int boostCooldown;//Variable used to display to the user how long the boost is on cooldown for.
	bool boostActive = false;//Bool used to determine whether the boost is active or not. 
	const float boostMaxSpeed = 120.f;//This is the max speed that the boost accelerates the car to.
	//Car move speed values. 
	float carMoveSpeed = 0.f;
	const float carMoveSpeedMax = 100.f;
	const float carMoveSpeedMin = -20.f;
	float carAcceleration = 0.05f;
	float carDeceleration = 0.025f;
	const float carEndDeceleration = 0.1f;
	float carDrag = 0.01f;//This is used to passively accelerate/decelerate the car whilst it is moving so that is passively slows down.
	//Camera.
	ICamera* camera = myEngine->CreateCamera(kManual);
	const float cameraMoveSpeed = 50.f;
	const float cameraRotation = 20.f;
	float cameraXPos = 0.f;
	float cameraYPos = 10.f;
	float cameraZPos = -20.f;
	//Variables used when checking if a collision has occured. 
	float collisionDistanceCheckpoint;
	float collisionDistanceTanks;
	float collsionDistanceCheckpointRightStrut;
	float collsionDistanceCheckpointLeftStrut;
	float collisionDistanceWaypoint;//Variable used by enemy car to determine when to look at the next waypoint. 
	float collisionDistanceCars;//Variable used to check for a collision between player and enemy car.

	bool firstPersonCamera = false;//Bool used to record whether the camera is in first person or third person.
	bool raceStart = false;//Bool used to record if the race has started.
	const float carRotateSpeed = 50.f;
	const float carDamageRotationSpeed = 10.f;

	//Creates the backdrop that is placed at the bottom of the screen and acts as part of the HUD.
	ISprite* backdrop = myEngine->CreateSprite(backdropModel, 0, 650);
	
	//Cross that is used to indicate successful stage completion.
	IMesh* crossMesh = myEngine->LoadMesh(crossModel);
	IModel* cross;
	float crossTimer = 0;
	bool crossCreated = false; 	
	const float crossSpawnTime = 0.5f;//Time that the cross is visible.
	//Skybox.
	IMesh* skyboxMesh = myEngine->LoadMesh(skyboxModel);
	IModel* skybox = skyboxMesh->CreateModel(0.0f,-960,0);	
	//Ground.
	IMesh* groundMesh = myEngine->LoadMesh(groundModel);
	IModel* ground = groundMesh->CreateModel();
	//Car.
	IMesh* carMesh = myEngine->LoadMesh(carModel);
	const int numCars = 2;
	const float carRadius = 2.f;//Radius of car used is 2, this is also the car width, if value was any bigger then the collision detection on the side of the car would be more inaccurate.
	const float carRadiusMultiply2 = 4.f;//Value used when checking for a collision between the enemy and player car.
	//Variables used to make the car hover up and down as if it were floating.
	float carYMovement;
	float carWave = 0;
	const float carWaveMultiplier = 250.f;
	const float carWaveMaxValue = 180.f;
	const float carWaveIncrement = 0.005f;

	cars car[numCars];
	car[0].car = carMesh->CreateModel(5,0,-20);	
	const int carDamageValue = 50;//Value at which the player car skin changes due to being damaged.
	car[1].car = carMesh->CreateModel(-5, 0, -30);
	car[1].car->SetSkin(enemyCarSkin);
	float enemyCarSpeeds[4] = { 50.f, 25.f, 75.f, 100.f };//Array of enemy car speeds, to allow it to change speed between check points.
	float enemyCarSpeed = enemyCarSpeeds[0];//Speed of enemy car.
	//Camera is attached to the player car, to act as a chase camera and it is placed behind and above the car.	
	camera->AttachToParent(car[0].car);
	camera->MoveLocal(cameraXPos, cameraYPos, cameraZPos);
	camera->RotateLocalX(cameraRotation);
	//Isles and walls.
	IMesh* islesMesh = myEngine->LoadMesh(islesModel);
	IModel* isles[numIsles];
	//Dimensions for both the isles and the walls, these are used in determining the size of the collision box for wall collisions.
	//We only need the isles width because the walls are placed in the middle of the isles model, hence the isle width can be used.
	const float isleWidth = 5.35058;
	const float isleLength = 6.83496;
	const float wallLength = 9.67118;
	const float totalWallWidth = isleWidth;
	const float totalWallLength = wallLength + isleLength;

	isles[0] = islesMesh->CreateModel(-10,0,40);
	isles[1] = islesMesh->CreateModel(10, 0, 40);
	isles[2] = islesMesh->CreateModel(-10, 0, 53);
	isles[3] = islesMesh->CreateModel(10, 0, 53);
	isles[4] = islesMesh->CreateModel(339.0, 0, 5.5954304);
	isles[5] = islesMesh->CreateModel(339.0, 0, -47.59543);
	isles[6] = islesMesh->CreateModel(381.0, 0, 5.5954304);
	isles[7] = islesMesh->CreateModel(381.0, 0, -47.59543);
	//Walls.
	IMesh* wallMesh = myEngine->LoadMesh(wallModel);
	IModel* walls[numWalls];
	walls[0] = wallMesh->CreateModel(-10.5, 0, 46);
	walls[1] = wallMesh->CreateModel(9.5, 0, 46);
	walls[2] = wallMesh->CreateModel(339.0, 0, -40.34236);
	walls[3] = wallMesh->CreateModel(339.0, 0, -30.67118);
	walls[4] = wallMesh->CreateModel(339.0, 0, -21.0);
	walls[5] = wallMesh->CreateModel(339.0, 0, -11.32882);
	walls[6] = wallMesh->CreateModel(339.0, 0, -1.6576405);
	walls[7] = wallMesh->CreateModel(381.0, 0, -40.34236);
	walls[8] = wallMesh->CreateModel(381.0, 0, -30.67118);
	walls[9] = wallMesh->CreateModel(381.0, 0, -21.0);
	walls[10] = wallMesh->CreateModel(381.0, 0, -11.32882);
	walls[11] = wallMesh->CreateModel(381.0, 0, -1.6576405);
	//Checkpoints.
	IMesh* checkpointMesh = myEngine->LoadMesh(checkpointModel);
	IModel* checkpoint[numCheckPoints];
	checkpoint[0] = checkpointMesh->CreateModel(0, 0, 0);
	checkpoint[1] = checkpointMesh->CreateModel(0, 0, 100);
	checkpoint[2] = checkpointMesh->CreateModel(58.82953, 0, 255.58012);
	checkpoint[2]->RotateLocalY(checkpoint2Rotation);
	checkpoint[3] = checkpointMesh->CreateModel(249.65405, 0, 267.0279);
	checkpoint[3]->RotateLocalY(checkpoint3Rotation);
	checkpoint[4] = checkpointMesh->CreateModel(360, 0, 14);
	checkpoint[5] = checkpointMesh->CreateModel(360, 0, -71);
	checkpoint[5]->RotateLocalY(checkpoint5Rotation);
	checkpoint[6] = checkpointMesh->CreateModel(120.051506, 0, -122.59522);
	checkpoint[6]->RotateLocalY(checkpoint6Rotation);
	//Waypoints used by enemy car to move around the track.
	IMesh* waypointMesh = myEngine->LoadMesh(dummyModel);
	IModel* waypoint[numWaypoints];
	const int lastWaypoint = 17;
	const float waypointRadius = 1.f;
	waypoint[0] = waypointMesh->CreateModel(0, 0, 0);
	waypoint[1] = waypointMesh->CreateModel(0, 0, 100);
	waypoint[2] = waypointMesh->CreateModel(0.40274048, 0, 129.65225);
	waypoint[3] = waypointMesh->CreateModel(20.211731, 0, 177.02203);
	waypoint[4] = waypointMesh->CreateModel(58.82953, 0, 255.58012);
	waypoint[5] = waypointMesh->CreateModel(116.43454, 0, 283.18097);
	waypoint[6] = waypointMesh->CreateModel(192.22089, 0, 287.00214);
	waypoint[7] = waypointMesh->CreateModel(249.65405, 0, 267.0279);
	waypoint[8] = waypointMesh->CreateModel(294.35004, 0, 230.49533);
	waypoint[9] = waypointMesh->CreateModel(352.00385, 0, 145.22859);
	waypoint[10] = waypointMesh->CreateModel(360.0, 0, 83.0);
	waypoint[11] = waypointMesh->CreateModel(360.0, 0, 14.0);
	waypoint[12] = waypointMesh->CreateModel(360.0, 0, -71.0);
	waypoint[13] = waypointMesh->CreateModel(329.8245, 0, -94.60072);
	waypoint[14] = waypointMesh->CreateModel(263.00583, 0, -122.0863);
	waypoint[15] = waypointMesh->CreateModel(191.86703, 0, -146.83514);
	waypoint[16] = waypointMesh->CreateModel(120.051506, 0, -122.59522);
	waypoint[17] = waypointMesh->CreateModel(76.98221, 0, -104.05701);
	//The enemy car starts off by looking at second waypoint.
	car[1].car->LookAt(waypoint[1]);
	//Tanks.
	IMesh* tankMesh2 = myEngine->LoadMesh(tank2Model);
	IModel* tank[numTanks];
	tank[0] = tankMesh2->CreateModel(303.02246, -5, 189.12762);
	tank[0]->RotateZ(20);
	tank[1] = tankMesh2->CreateModel(241.02248, 0, 194.12762);
	tank[2] = tankMesh2->CreateModel(257.02246, 0, 134.12762);
	tank[3] = tankMesh2->CreateModel(379.02246, 0, 148.12762);
	tank[4] = tankMesh2->CreateModel(354.02246, 0, 216.12762);
	tank[5] = tankMesh2->CreateModel(377.02246, 0, -152.87238);
	tank[6] = tankMesh2->CreateModel(304.02246, 0, -182.87238);
	tank[7] = tankMesh2->CreateModel(211.02248, 0, -200.87238);
	tank[8] = tankMesh2->CreateModel(117.02247, 0, -193.87238);
	tank[9] = tankMesh2->CreateModel(32.02247, 0, -152.87238);
	tank[10] = tankMesh2->CreateModel(313.02246, 0, 257.12762);
	tank[11] = tankMesh2->CreateModel(-27.97753, 0, -101.87238);
	tank[12] = tankMesh2->CreateModel(-79.97753, 0, 176.12762);
	tank[13] = tankMesh2->CreateModel(-22.97753, 0, 259.12762);
	tank[14] = tankMesh2->CreateModel(70.755, 0, 334.999);
	tank[15] = tankMesh2->CreateModel(194.516, 0, 342.493);	
	//Allow game to check if keys are pressed/held.
	bool accelerateKeyPressed = false; //W Key, speeds the car up.
	bool decelerateKeyPressed = false;//S Key, slows the car down.
	bool turnRightKeyPressed = false;//D Key, turns the car clockwise.
	bool turnLeftKeyPressed = false;//A Key, turns the car anti clockwise.
	bool startGameKeyPressed = false;//Space bar, starts the game.
	bool moveCameraForwardKeyPressed = false;//Up Arrow, moves camera forward.
	bool moveCameraBackKeyPressed = false;//Down Arrow, moves camera backwards.
	bool moveCameraRightKeyPressed = false;//Right Arrow, moves camera right.
	bool moveCameraLeftKeyPressed = false;//Left Arrow, moves camera left.
	bool resetCameraKeyPressed = false;//1 Key, resets the camera to third person viewpoint.
	bool firstPersonCameraKeyPressed = false;//2 Key, moves the camera to a first person viewpoint.
	bool boostKeyPressed = false;//Space bar when it is held down activates the boost.

	// The main game loop, repeat until engine is stopped
	while (myEngine->IsRunning())
	{
		//Frame time is used to ensure movement is constant on any computer speed, and it is also used as a timer for certain game aspects.
		float frameTime = myEngine->Timer();
		textCounter += frameTime;
		crossTimer += frameTime;
		
		//Code used to control the countdown at the start of the game, each number is displayed for one second and then the next number is displayed.
		if (textCounter >= 1.f && textState == NoTextBefore)
		{
			textState = Three;
			textCounter = 0;//The text counter is reset after each number has been displayed.
		}
		if (textCounter >= 1.f && textState == Three)
		{
			textState = Two;
			textCounter = 0;
		}
		if (textCounter >= 1.f && textState == Two)
		{
			textState = One;
			textCounter = 0;
		}
		if (textCounter >= 1.f && textState == One)
		{
			textState = Go;
			textCounter = 0;
		}
		if (textCounter >= 1.f && textState == Go)
		{
			drawText = !drawText;
			textState = NoTextAfter;
			textCounter = 0;
		}
	
		// Draw the scene
		myEngine->DrawScene();

		/**** Update your scene each frame here ****/

		//stringstreams used to display information to the player.
		stringstream stateOfGame;//This displayed what stage the player is in.
		stringstream speed;//Displays the speed of the car.
		stringstream health;//Displays car health.
		stringstream boost;//Displays information regarding boost, such as it's remaining cooldown.
		stringstream playerTime;//Stores and displays the players race time.
		stringstream enemyTime;//Stores the enemy player race time, used after the race to determine who won.
		stringstream playerWin;//Used to display the players time if they win the race.
		stringstream enemyWin;//Used to display the enemy time if the player loses the race.	
		//This records the race time for the enemy and the player.
		playerTime << "Race Time: " << car[0].raceTimer << " seconds";
		enemyTime << car[1].raceTimer;
		//Health of the car is stored and displayed on the HUD.
		health << "Health " << car[0].health;
		//Car speed is set to be displayed using 3 s.f.
		speed.precision(3);
		speed << "Speed " << carMoveSpeed;
		//This is used to control the rotation of the camera using the mouse movements.
		int mouseMoveX = myEngine->GetMouseMovementX();
		camera->RotateY(mouseMoveX * frameTime * cameraMoveSpeed);
		int mouseMoveY = myEngine->GetMouseMovementY();
		camera->RotateX(mouseMoveY * frameTime * cameraMoveSpeed);
		
		//Point at which game starts.
		if (overallGameState == start)
		{			
			stateOfGame << ready;
			myFont->Draw(startString, 650, 100, kRed, kCentre);
			//Check to see if space bar has been pressed, if so then the game countdown starts.
			if (startGameKeyPressed)
			{
				overallGameState = racing;
			}
		}
		//The game is then in stage one.
		if (overallGameState == racing)
		{
			stateOfGame << stage1;
		}
		//If the game is finished then this is displayed on the HUD.
		if (overallGameState == raceEnd)
		{
			stateOfGame << raceOver;
		}
		//Allows the user to exit the game using the escape key, instead of Alt+F4.
		if (myEngine->KeyHit(Key_Escape))
		{
			myEngine->Stop();
		}
		/*Variables used to monitor when keys are hit.
		Change bool values to enable/disable function of the keys.*/
		accelerateKeyPressed = (myEngine->KeyHeld(Key_W));
		decelerateKeyPressed = (myEngine->KeyHeld(Key_S));
		turnLeftKeyPressed = (myEngine->KeyHeld(Key_A));
		turnRightKeyPressed = (myEngine->KeyHeld(Key_D));
		startGameKeyPressed = (myEngine->KeyHit(Key_Space));
		boostKeyPressed = (myEngine->KeyHeld(Key_Space));
		moveCameraForwardKeyPressed = (myEngine->KeyHeld(Key_Up));
		moveCameraBackKeyPressed = (myEngine->KeyHeld(Key_Down));
		moveCameraLeftKeyPressed = (myEngine->KeyHeld(Key_Left));
		moveCameraRightKeyPressed = (myEngine->KeyHeld(Key_Right));
		resetCameraKeyPressed = (myEngine->KeyHit(Key_1));
		firstPersonCameraKeyPressed = (myEngine->KeyHit(Key_2));	
		//Camera controls are only enabled when the camera is in third person, this is to prevent the user from moving the camera into the car.
		if (!firstPersonCamera)
		{
			if (moveCameraForwardKeyPressed)
			{
				camera->MoveLocalZ(cameraMoveSpeed * frameTime);
			}
			if (moveCameraBackKeyPressed)
			{
				camera->MoveLocalZ(-cameraMoveSpeed * frameTime);
			}
			if (moveCameraLeftKeyPressed)
			{
				camera->MoveLocalX(-cameraMoveSpeed * frameTime);
			}
			if (moveCameraRightKeyPressed)
			{
				camera->MoveLocalX(cameraMoveSpeed * frameTime);
			}
		}
		//Check if the user wants to switch to first person camera.
		if (firstPersonCameraKeyPressed)
		{
			firstPersonCamera = !firstPersonCamera;
			FirstPersonCamera(camera, car[0].car);
		}
		if (resetCameraKeyPressed)
		{
			firstPersonCamera = !firstPersonCamera;
			ResetCamera(camera, car[0].car, cameraRotation);
		}	
		//RACE BEGINS HERE.
		if (overallGameState == racing)
		{	
			//This section is used to display the starting countdown.
			if (drawText && textState == Three)
			{
				myFont->Draw("3", 650, 100, kRed, kCentre);
			}
			
			if (drawText && textState == Two)
			{
				myFont->Draw("2", 650, 100, kRed, kCentre);
			}

			if (drawText && textState == One)
			{
				myFont->Draw("1", 650, 100, kRed, kCentre);
			}

			if (drawText && textState == Go)
			{
				myFont->Draw(goString, 650, 100, kRed, kCentre);
				raceStart = true;//This bool is used to prevent the user from moving the car until the countdown has ended.
				//Cannot use raceStart = !raceStart, because this could result in the game becoming locked up and prevent the user from playing the game.
			}
			if (raceStart)
			{
				//These 2 blocks are used to record the race time for each car as long as the car hasn't finished the race.
				if (!car[0].raceFinished)
				{
					car[0].raceTimer += frameTime;
				}
				if (!car[1].raceFinished)
				{
					car[1].raceTimer += frameTime;
				}
				//Car movement.
				car[0].car->MoveLocalZ(carMoveSpeed * frameTime);
				car[1].car->MoveLocalZ(enemyCarSpeed * frameTime);
				//This is used to move the hover car up and down to make it hover.
				//The car moves up and down along a sine wave.
				carWave += carWaveIncrement;
				carYMovement = sin(carWave);
				car[0].car->SetY(carYMovement * frameTime * carWaveMultiplier);
				car[1].car->SetY(carYMovement*frameTime * carWaveMultiplier);
				//The value will only increment to a maximum of 180, to ensure that the car doesn't sink below the ground.
				//This would occur if the car moved along the whole length of a sine wave not just the first half.
				if (carWave == carWaveMaxValue)
				{
					carWave = 0;
				}
				//The car slows down/speeds up passively regardless of the accelerate/decelerate buttons being pressed. 
				//This gives the player car momentum and that it will gradually slow down to a stop.
				if (carMoveSpeed > 0)
				{				
					carMoveSpeed -= carDrag;
				}

				if (carMoveSpeed < 0)
				{	
					carMoveSpeed += carDrag;
				}
				//This accelerates the car to its max base speed limit.
				if (accelerateKeyPressed && carMoveSpeed < carMoveSpeedMax)
				{					
					carMoveSpeed += carAcceleration;
				}
				//This decelerates the car to its min speed limit.
				if (decelerateKeyPressed && carMoveSpeed >= carMoveSpeedMin)
				{
					carMoveSpeed -= carDeceleration;
				}
				//Car rotations.
				if (turnLeftKeyPressed)
				{
					car[0].car->RotateLocalY(-carRotateSpeed * frameTime);
				}
				if (turnRightKeyPressed)
				{
					car[0].car->RotateLocalY(carRotateSpeed * frameTime);
				}
				//Checks whether the boost key is pressed, if so then the boost is activated.
				if (boostKeyPressed)
				{
					boostActive = true;
				}
				//SPEED BOOST.
				if (boostActive)
				{
					boostCounter += frameTime;//Records how long the boost is active.
					if (boostCounter < boostDuration)
					{
						myFont->Draw(boostActiveString, 650, 50, kRed, kCentre);//Lets the user know the boost is active by providing an on screen message. 
						//Accelerates the car rapidly up to the max boost speed.
						if (carMoveSpeed <= boostMaxSpeed)
						{
							carMoveSpeed += boostSpeedChange;
						}
					}
					//1 second before the boost overheats, a warning is displayed on screen.
					if (boostCounter > boostAccelerationStartWarning && boostCounter < boostAccelerationStopWarning)
					{
						myFont->Draw(boostOverheatString, 650, 100, kRed, kCentre);
					}
					//Then the boost stops and the car rapidly decelerates over 1 second.
					if (boostCounter > boostAccelerationStopWarning && boostCounter < boostPeriodOver)
					{						
						if (boostCounter > boostAccelerationStopWarning && boostCounter < boostDecelerationPeriod && carMoveSpeed > carMoveSpeedMin)
						{
							carMoveSpeed -= boostSpeedChange;
						}
						//The boost is then disabled for 5 seconds.
						boostKeyPressed = false;
						boostCooldown = boostPeriodOver - boostCounter;
						boost << "Boost unavailable for " << boostCooldown << " seconds";
						myFont->Draw(boost.str(), 650, 50, kRed, kCentre);
					}
					//Once the boost has cooled down, it is ready for use again.
					if (boostCounter >= boostPeriodOver)
					{
						boostCounter = 0;
						boostActive = !boostActive;
					}
				}
				//Loop used to guide enemy car around the race track.
				//The enemy car moves around the track by looking at each waypoint and once it reaches the waypoint its currently looking at
				//it looks at the next waypoint and moves to that one.
				for (int i = 0; i < numWaypoints; i++)
				{									
						collisionDistanceWaypoint = CollisionDetection(waypoint[i], car[1].car);
						if (!car[1].raceFinished)
						{
							//Last waypoint is situated slightly past the last checkpoint to ensure the enemy car doesnt block the checkpoint.
							//Once the enemy reaches this checkpoint its race timer stops and it stops still.
							if (collisionDistanceWaypoint < waypointRadius && (i + 1) == numWaypoints)
							{
									car[1].raceFinished = true;																						
									enemyCarSpeed = 0;								
							}
							//Every time the car reaches the current waypoint it's looking at, it looks at the next check point and moves towards it.
							//This continues around the whole track.
							if (collisionDistanceWaypoint < waypointRadius && (i + 1) < numWaypoints)
							{
								car[1].car->LookAt(waypoint[i + 1]);
								//This section is used to vary the speed of the enemy car as it passes certain check points.
								if (i == waypoint4)
								{
									enemyCarSpeed = enemyCarSpeeds[1];
								}
								if (i == waypoint8)
								{
									enemyCarSpeed = enemyCarSpeeds[2];
								}
								if (i == waypoint12)
								{
									enemyCarSpeed = enemyCarSpeeds[3];
								}
							}
						}				
				}
				//CHECKPOINT/PLAYER car collision.
				//To ensure that the race cannot be completed out of order, a checkpoint can only be completed, if the checkpointState variable
				//is equal to the stage before the current checkpoint you are trying to pass through.
				for (int i = 0; i < numCheckPoints; i++)
				{					
					collisionDistanceCheckpoint = CollisionDetection(checkpoint[i], car[0].car);
					if (collisionDistanceCheckpoint < carRadiusAddCheckpointRadius)
					{									
						if (i == numCheckPoints - 1)
						{
							//We must check the checkpoint state to see if the player has completed the checkpoint before the last checkpoint.
							if (checkpointState == checkpointStates(numCheckpointStates - 1))
							{
								crossTimer = 0;
								DisplayCross(checkpoint[i], cross, crossMesh, i);
								car[0].raceFinished = true;
								crossCreated = true;
								checkpointState = stateOver;														
							}						
						}
						//As the player passes through a checkpoint, if it is valid then the checkpoint state enum, updates to allow the user to progress through the race.
						if (i == 0)
						{
							if (checkpointState == checkpointStates(i))
							{
								crossTimer = 0;//We must reset the cross timer to ensure that the crosses will be deleted after 0.5 seconds.
								checkpointState = checkpointStates(i + 1);//Change the checkpoint state to allow the race to be progressed through.
								DisplayCross(checkpoint[i], cross, crossMesh, i);//Create a cross to indicate successfull stage completiopn 
								crossCreated = true;//Change bool to indicate that a cross is present. This is needed to prevent the game from trying to delete a cross that might not be there.
							}
						}			
						if (i > 0 && i != numCheckPoints - 1)
						{						
							if (checkpointState == checkpointStates(i))
							{			
								crossTimer = 0;
								DisplayCross(checkpoint[i], cross, crossMesh, i);
								crossCreated = true;
								checkpointState = checkpointStates(i + 1);										
							}
						}		
					}

				}
				//Both cars must have finished the race for it to end and for the state to swap.
				if (car[0].raceFinished && car[1].raceFinished)
				{
					overallGameState = raceEnd;
				}
				//This is used to delete the crosses that are placed above the checkpoints.
				if (crossCreated && crossTimer >= crossSpawnTime)
				{
					crossMesh->RemoveModel(cross);
					crossTimer = 0;
					crossCreated = !crossCreated;//This bool is set to false to prevent the game from trying to delete more crosses.
				}
				//CAR/CAR collision detection.
				collisionDistanceCars = CollisionDetection(car[1].car, car[0].car);
				if (collisionDistanceCars < carRadiusMultiply2)
				{
					//The player car bounces off the enemy car and loses 1 point of health.
					carMoveSpeed = -carMoveSpeed; 
					car[0].health--;
				}
				//CAR/TANK collision detection.
				for (int i = 0; i < numTanks; i++)
				{
					collisionDistanceTanks = CollisionDetection(tank[i], car[0].car);
					if (collisionDistanceTanks < carRadiusAddTankRadius)
					{
						//The player car bounces off the tank and loses 1 point of health.
						car[0].health--;
						carMoveSpeed = -carMoveSpeed;
					}
				}
				//CAR/WALL collision detection.
				for (int i = 0; i < numWalls; i++)
				{

					sphereBoxCollision = sphereBoxCollisionDetection(car[0].car, walls[i], carRadius, 1, totalWallLength);
					if (sphereBoxCollision == true)
					{
						//The player car bounces off the walls and loses 1 point of health.
						car[0].health--;
						carMoveSpeed = -carMoveSpeed;
					}

				}
				//CAR/CHECKPOINT STRUT collision detection.
				for (int i = 0; i < numCheckPoints; i++)
				{			
					collsionDistanceCheckpointRightStrut = CollisionDetectionCheckpointRightStrut(checkpoint[i], car[0].car, i);
					collsionDistanceCheckpointLeftStrut = CollisionDetectionCheckpointLeftStrut(checkpoint[i], car[0].car, i);
				
					if (collsionDistanceCheckpointRightStrut <= carRadiusAddStrutRadius || collsionDistanceCheckpointLeftStrut <= carRadiusAddStrutRadius)
					{				
						//The player car bounces off the checkpoint strut and loses 1 point of health.
						car[0].health--;
						carMoveSpeed = -carMoveSpeed;
					}

				}
				//Once the car loses 50 points of health, its skin will change to indicate it has become damaged.
				//The car will also become harder to steer, and it will passively pull towards the left.
				if (car[0].health <= carDamageValue)
				{
					car[0].car->RotateY(-carDamageRotationSpeed * frameTime);
					car[0].car->SetSkin(carDamageSkin1);
				}
				//If the car health falls below or equal to 0 then the game is over, and the player loses.
				if (car[0].health <= 0)
				{
					overallGameState = raceEnd;
				}
			}
		}
		//Code used to change the information displayed to the user to indicate what stage they are currently in.
		if (checkpointState == state2)
		{
			stateOfGame.str(""); // Clear myStream
			stateOfGame << "Stage 2";
		}
		if (checkpointState == state3)
		{
			stateOfGame.str(""); // Clear myStream
			stateOfGame << "Stage 3";	
		}
		if (checkpointState == state4)
		{
			stateOfGame.str(""); // Clear myStream
			stateOfGame << "Stage 4";
		}
		if (checkpointState == state5)
		{
			stateOfGame.str(""); // Clear myStream
			stateOfGame << "Stage 5";
		}
		if (checkpointState == state6)
		{
			stateOfGame.str(""); // Clear myStream
			stateOfGame << "Stage 6";
		}
		if (checkpointState == state7)
		{
			stateOfGame.str(""); // Clear myStream
			stateOfGame << "Stage 7";
		}
		if (checkpointState == stateOver)
		{
			stateOfGame.str(""); // Clear myStream
			stateOfGame << "Race Over";
		}
		//Point at which the race is over.
		if (overallGameState == raceEnd)
		{
			//If the player car has 0 health then they lose.
			if (car[0].health <= 0)
			{
				myFont->Draw(carDestroyed, 650, 200, kMagenta, kCentre);
			}
			//If the player completes the race before the enemy car, then the player wins.
			if (car[0].raceTimer < car[1].raceTimer && car[0].health > 0)
			{
				myFont->Draw(victory, 650, 200, kMagenta, kCentre);
				playerWin << "With  a time of " << car[0].raceTimer;
				myFont->Draw(playerWin.str(), 650, 300, kMagenta, kCentre);
			}  
			//If the player completes the race after the enemy car, then the player loses..
			if (car[0].raceTimer > car[1].raceTimer && car[0].health > 0)
			{
				myFont->Draw(defeat, 650, 200, kMagenta, kCentre);
				enemyWin << "Enemy won with  a time of " << car[1].raceTimer;
				myFont->Draw(enemyWin.str(), 650, 300, kMagenta, kCentre);
			}
			myFont->Draw(gameOverEndString, 650, 100, kRed, kCentre);
			//This section brings the player car to a gradual stop.
			car[0].car->MoveLocalZ(carMoveSpeed*frameTime);
			if (carMoveSpeed > 0)
			{
				carMoveSpeed -= carEndDeceleration;
			}
			if (carMoveSpeed <= 0)
			{
				carMoveSpeed = 0;
			}	
			//This is used to remove the cross that appears over the last checkpoint.
			if (crossCreated && crossTimer >= crossSpawnTime)
			{
				crossMesh->RemoveModel(cross);
				crossTimer = 0;
				crossCreated = false;
			}		
		}
		//This is used to display all information as a HUD, that will be present on the background model along the bottom of the screen.
		HUDFont->Draw(stateOfGame.str(), 0, 650);
		HUDFont->Draw(playerTime.str(), 400, 680);
		HUDFont->Draw(speed.str(), 0, 680);
		HUDFont->Draw(health.str(), 200, 680);
	}
	// Delete the 3D engine now we are finished with it
	myEngine->Delete();
}
//This function is used to reset the camera to its starting position of a third person viewpoint.
void ResetCamera(ICamera* camera, IModel* car, float rotation)
{
	float x = car->GetLocalX();
	float y = car->GetLocalY();
	float z = car->GetLocalZ();
	camera->SetPosition(x, y, z);
	camera->ResetOrientation();
	//Use local commands since camera is attached to the car.
	camera->RotateLocalX(rotation);
	camera->MoveLocal(0, normalCameraY, normalCameraZ);
}
//Function used to switch to a first person viewpoint that is positioned in front of the car.
void FirstPersonCamera(ICamera* camera, IModel* car)
{
	float x = car->GetLocalX();
	float y = car->GetLocalY();
	float z = car->GetLocalZ();
	camera->SetPosition(x, y, z);
	camera->ResetOrientation();
	//Use local commands since camera is attached to the car.
	camera->MoveLocal(0, fpCameraY, fpCameraZ);
}
//Function used to detect sphere/sphere collision detection.
float CollisionDetection(IModel* object1, IModel* object2)
{
	float x;
	float y;
	float z;
	//Calculate the vector between the bullet and the mushrooms.
	x = object1->GetX() - object2->GetX();
	y = object1->GetY() - object2->GetY();
	z = object1->GetZ() - object2->GetZ();
	//Then calculate the length of the vector/distance between the two objects.
	float collisionDist = sqrt(x*x + y*y + z*z);
	return collisionDist;
}
//Function used to detect collision between player car and the right strut of each checkpoint.
float CollisionDetectionCheckpointRightStrut(IModel* stationaryObject, IModel* movingObject, int checkpointNumber)
{
	float x;
	float y;
	float z;
	float collisionDist;
	//Some of the checkpoints are rotated, and so the struts aren't in a straight line.
	//This means the collision areas must be offset so that they are placed where the struts are.
	//Each checkpoint requires a different offset, in varying axis due to the difference in their rotation.
	const float check3RightXOffset = 6.5f;
	const float check3RightZOffset = 5.f;
	const float check2ZOffset = 5.5f;
	const float check6ZOffset = 6.f;
	if (checkpointNumber == checkpointNum3)
	{
		//Calculate the vector between the bullet and the mushrooms.
		x = stationaryObject->GetLocalX() - check3RightXOffset - movingObject->GetX();
		y = stationaryObject->GetY() - movingObject->GetY();
		z = stationaryObject->GetLocalZ() - check3RightZOffset - movingObject->GetZ();
		//Then calculate the length of the vector/distance between the two objects.
		collisionDist = sqrt(x*x + y*y + z*z);
	}
	if (checkpointNumber == checkpointNum2)
	{
		//Calculate the vector between the bullet and the mushrooms.
		x = stationaryObject->GetLocalX() + check3RightXOffset - movingObject->GetX();
		y = stationaryObject->GetY() - movingObject->GetY();
		z = stationaryObject->GetLocalZ() - check2ZOffset - movingObject->GetZ();
		//Then calculate the length of the vector/distance between the two objects.
		collisionDist = sqrt(x*x + y*y + z*z);
	}
	if (checkpointNumber == checkpointNum6)
	{
		//Calculate the vector between the bullet and the mushrooms.
		x = stationaryObject->GetLocalX() + check3RightXOffset - movingObject->GetX();
		y = stationaryObject->GetY() - movingObject->GetY();
		z = stationaryObject->GetLocalZ() + check6ZOffset - movingObject->GetZ();
		//Then calculate the length of the vector/distance between the two objects.
		collisionDist = sqrt(x*x + y*y + z*z);
	}
	else if (checkpointNumber == checkpointNum0 || checkpointNumber == checkpointNum1 || checkpointNumber == checkpointNum4 || checkpointNumber == checkpointNum5)
	{
		//Calculate the vector between the bullet and the mushrooms.
		x = stationaryObject->GetLocalX() + checkpointWidth - movingObject->GetX();
		y = stationaryObject->GetY() - movingObject->GetY();
		z = stationaryObject->GetZ() - movingObject->GetZ();
		//Then calculate the length of the vector/distance between the two objects.
		collisionDist = sqrt(x*x + y*y + z*z);
	}
	return collisionDist;
}
//Function used to detect collision between player car and the left strut of each checkpoint.
float CollisionDetectionCheckpointLeftStrut(IModel* stationaryObject, IModel* movingObject, int checkpointNumber)
{
	float x;
	float y;
	float z;
	float collisionDist;
	//Some of the checkpoints are rotated, and so the struts aren't in a straight line.
	//This means the collision areas must be offset so that they are placed where the struts are.
	//Each checkpoint requires a different offset, in varying axis due to the difference in their rotation.
	const float checkpointZOffset = 9.f;
	const float checkpoint2XOffset = 2.f;
	const float checkpoint3And6XOffset = 5.f;
	if (checkpointNumber == checkpointNum2)
	{
		//Calculate the vector between the bullet and the mushrooms.
		x = stationaryObject->GetLocalX() - checkpoint2XOffset - movingObject->GetX();
		y = stationaryObject->GetY() - movingObject->GetY();
		z = stationaryObject->GetLocalZ() + checkpointZOffset - movingObject->GetZ();
		//Then calculate the length of the vector/distance between the two objects.
		collisionDist = sqrt(x*x + y*y + z*z);
	}
	if (checkpointNumber == checkpointNum3)
	{
		//Calculate the vector between the bullet and the mushrooms.
		x = stationaryObject->GetLocalX() + checkpoint3And6XOffset - movingObject->GetX();
		y = stationaryObject->GetY() - movingObject->GetY();
		z = stationaryObject->GetLocalZ() + checkpointZOffset - movingObject->GetZ();
		//Then calculate the length of the vector/distance between the two objects.
		collisionDist = sqrt(x*x + y*y + z*z);
	}
	if (checkpointNumber == checkpointNum6)
	{
		//Calculate the vector between the bullet and the mushrooms.
		x = stationaryObject->GetLocalX() - checkpoint3And6XOffset - movingObject->GetX();
		y = stationaryObject->GetY() - movingObject->GetY();
		z = stationaryObject->GetLocalZ() - checkpointZOffset - movingObject->GetZ();
		//Then calculate the length of the vector/distance between the two objects.
		collisionDist = sqrt(x*x + y*y + z*z);
	}			
	else if (checkpointNumber == checkpointNum0 || checkpointNumber == checkpointNum1 || checkpointNumber == checkpointNum4 || checkpointNumber == checkpointNum5)
	{
		//Calculate the vector between the bullet and the mushrooms.
		x = stationaryObject->GetLocalX() - checkpointWidth - movingObject->GetX();
		y = stationaryObject->GetY() - movingObject->GetY();
		z = stationaryObject->GetZ() - movingObject->GetZ();
		//Then calculate the length of the vector/distance between the two objects.
		collisionDist = sqrt(x*x + y*y + z*z);
	}
	return collisionDist;
}
//Function used to check for a collision between the player car and the walls.
bool sphereBoxCollisionDetection(IModel* sphere, IModel* box, float sphereRad, float boxWidth, float boxLength)
{
	float wallMaxXlimit;
	float wallMinXLimit;
	float wallMinZLimit;
	float wallMaxZLimit;	
	for (int i = 0; i < numWalls; i++)
	{
		wallMaxXlimit = box->GetX() + (boxWidth / 2) + sphereRad;
		wallMinXLimit = box->GetX() - (boxWidth / 2) - sphereRad;

		wallMinZLimit = box->GetZ() - (boxLength / 2) - sphereRad;
		wallMaxZLimit = box->GetZ() + (boxLength / 2) + sphereRad;
		//If there is a collision then we return true.
		if (sphere->GetX() > wallMinXLimit && sphere->GetX() < wallMaxXlimit && sphere->GetZ() > wallMinZLimit && sphere->GetZ() < wallMaxZLimit)
		{
			return true;
		}
	}
}
//Function used to display a cross to indicate a successful stage completion.
void DisplayCross(IModel* checkpoint, IModel* &cross, IMesh* crossMesh, int checkpointNumber)
{
	float x = checkpoint->GetLocalX();
	float y = checkpoint->GetY() + crossCheckpointYOffest;//The cross is offset in the Y axis to make it appear at the top of the checkpoint.
	float z = checkpoint->GetLocalZ();
	//Each cross must be rotated if the checkpoint has been rotated.
	if (checkpointNumber == checkpointNum2)
	{
		cross = crossMesh->CreateModel(x, y, z);
		cross->RotateY(checkpoint2Rotation); 
	}
	if (checkpointNumber == checkpointNum3)
	{
		cross = crossMesh->CreateModel(x, y, z);
		cross->RotateY(checkpoint3Rotation);
	}
	if (checkpointNumber == checkpointNum5)
	{
		cross = crossMesh->CreateModel(x, y, z);
		cross->RotateY(checkpoint5Rotation);
	}
	if (checkpointNumber == checkpointNum6)
	{
		cross = crossMesh->CreateModel(x, y, z);
		cross->RotateY(checkpoint6Rotation);
	}
	else if (checkpointNumber == checkpointNum0 || checkpointNumber == checkpointNum1 || checkpointNumber == checkpointNum4)
	{
		cross = crossMesh->CreateModel(x, y, z);
	}
	cross->Scale(crossScaleFactor);//The cross is scaled down to make it an appropriate size.
}


