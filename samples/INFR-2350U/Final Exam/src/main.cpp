//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);
	
	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 5.0f);
		glm::vec3 lightCol = glm::vec3(0.9f, 0.85f, 0.5f);
		float     lightAmbientPow = 0.05f;
		float     lightSpecularPow = 1.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 0.1f;
		float     lightLinearFalloff = 0.09f;
		float     lightQuadraticFalloff = 0.032f;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		//Bloom


		PostEffect* basicEffect;

		int activeEffect = 0;
		std::vector<PostEffect*> effects;

		SepiaEffect* sepiaEffect;
		GreyscaleEffect* greyscaleEffect;
		ColorCorrectEffect* colorCorrectEffect;
		BloomEffect* bloomEffect; //midterm
		DepthEffect* depthEffect; //Final
		/*
		// Creating an empty texture
		Texture2DDescription desc2 = Texture2DDescription();
		desc2.Width = 1;
		desc2.Height = 1;
		desc2.Format = InternalFormat::RGB8;
		Texture2D::sptr texture3 = Texture2D::Create(desc2);
		// Clear it with a white colour
		texture3->Clear();
		Texture2D::sptr blank = Texture2D::LoadFromFile("images/no_texture.jpg");
		*/

		int isTextureEnabled = 1; //1 = true 0 = false

		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Effect controls"))
			{
				ImGui::SliderInt("Chosen Effect", &activeEffect, 0, effects.size() - 1);

				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: Sepia Effect");

					SepiaEffect* temp = (SepiaEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Greyscale Effect");
					
					GreyscaleEffect* temp = (GreyscaleEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Color Correct Effect");

					ColorCorrectEffect* temp = (ColorCorrectEffect*)effects[activeEffect];
					static char input[BUFSIZ];
					ImGui::InputText("Lut File to Use", input, BUFSIZ);

					if (ImGui::Button("SetLUT", ImVec2(200.0f, 40.0f)))
					{
						temp->SetLUT(LUT3D(std::string(input)));
					}
				}
				if (activeEffect == 3) //midterm
				{
					ImGui::Text("Active Effect: Bloom Effect");

					BloomEffect* temp = (BloomEffect*)effects[activeEffect];
					float threshold = temp->GetThreshold(); // light threshold
					float downscale = temp->GetDownscale(); // downscale strength
					

					if (ImGui::SliderFloat("Threshold", &threshold, 0.0f, 0.99f))
					{
						temp->SetThreshold(threshold);
					}
					
					if (ImGui::SliderFloat("Downscale", &downscale, 1.0f, 5.0f))
					{
						temp->SetDownscale(downscale);
					}
					
				}
				if (activeEffect == 4) //Final
				{
					ImGui::Text("Active Effect: Depth Effect");

					DepthEffect* temp = (DepthEffect*)effects[activeEffect];
					float maximum = temp->GetMaximum(); // maximum
					float minimum = temp->GetMinimum(); // minimum 


					if (ImGui::SliderFloat("Maximum", &maximum, 5.0f, 10.0f))
					{
						temp->SetMaximum(maximum);
					}

					if (ImGui::SliderFloat("Minimum", &minimum, 0.0f, 4.99f))
					{
						temp->SetMinimum(minimum);
					}

				}
			}
			if (ImGui::CollapsingHeader("Environment generation"))
			{
				if (ImGui::Button("Regenerate Environment", ImVec2(200.0f, 40.0f)))
				{
					EnvironmentGenerator::RegenerateEnvironment();
				}
			}
			if (ImGui::CollapsingHeader("Scene Level Lighting Settings"))
			{
				if (ImGui::ColorPicker3("Ambient Color", glm::value_ptr(ambientCol))) {
					shader->SetUniform("u_AmbientCol", ambientCol);
				}
				if (ImGui::SliderFloat("Fixed Ambient Power", &ambientPow, 0.01f, 1.0f)) {
					shader->SetUniform("u_AmbientStrength", ambientPow);
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Pos", glm::value_ptr(lightPos), 0.01f, -10.0f, 10.0f)) {
					shader->SetUniform("u_LightPos", lightPos);
				}
				if (ImGui::ColorPicker3("Light Col", glm::value_ptr(lightCol))) {
					shader->SetUniform("u_LightCol", lightCol);
				}
				if (ImGui::SliderFloat("Light Ambient Power", &lightAmbientPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				}
				if (ImGui::SliderFloat("Light Specular Power", &lightSpecularPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				}
				if (ImGui::DragFloat("Light Linear Falloff", &lightLinearFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
				}
				if (ImGui::DragFloat("Light Quadratic Falloff", &lightQuadraticFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
				}
			}
			//toggle textures on/off
			if (ImGui::CollapsingHeader("Toggle textures"))
			{
				if (ImGui::Button("turn off Textures", ImVec2(200.0f, 40.0f))) // turn off textures
				
				{
					// Disable texturing
					
					isTextureEnabled = 0;
					
				}
				if (ImGui::Button("turn on Textures", ImVec2(200.0f, 40.0f))) // turn on textures

				{
					// Enable texturing
					isTextureEnabled = 1;


				}

			}

			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");
		
			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE); //disable
		glDepthFunc(GL_LEQUAL); // New 

		#pragma region TEXTURE LOADING

		// Load some textures from files
		Texture2D::sptr stone = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr stoneSpec = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr noSpec = Texture2D::LoadFromFile("images/grassSpec.png");
		Texture2D::sptr box = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr boxSpec = Texture2D::LoadFromFile("images/box-reflections.bmp");
		Texture2D::sptr simpleFlora = Texture2D::LoadFromFile("images/SimpleFlora.png");
		//New Midterm
		//tank
		Texture2D::sptr trackAndGun = Texture2D::LoadFromFile("images/chaffee_tracks_and_gun_color.jpg");
		Texture2D::sptr primaryColor = Texture2D::LoadFromFile("images/chaffee_1st_color.jpg");
		Texture2D::sptr secondaryColor = Texture2D::LoadFromFile("images/chaffee_2nd_color.jpg");
		Texture2D::sptr armorPack = Texture2D::LoadFromFile("images/chaffee_armor.jpg");
		Texture2D::sptr backLight = Texture2D::LoadFromFile("images/chaffee_light_b.jpg");
		Texture2D::sptr frontLight = Texture2D::LoadFromFile("images/chaffee_light_f.jpg");
		Texture2D::sptr frontWindow = Texture2D::LoadFromFile("images/chaffee_window.jpg");
		//taiga tree
		Texture2D::sptr pineLeaf = Texture2D::LoadFromFile("images/Taiga_tree_1_leaf.jpg");
		Texture2D::sptr pineWood = Texture2D::LoadFromFile("images/Taiga_tree_1_wood.jpg");
		//Finals
		Texture2D::sptr terrainDirt = Texture2D::LoadFromFile("images/terrain_dirt.jpg");
		Texture2D::sptr terrainGrass = Texture2D::LoadFromFile("images/terrain_grass.jpg");

		
		//blank texture
		Texture2D::sptr blankTexture = Texture2D::LoadFromFile("images/no_texture.jpg");
		LUT3D testCube("cubes/BrightenedCorrection.cube");

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr stoneMat = ShaderMaterial::Create();  
		stoneMat->Shader = shader;
		stoneMat->Set("s_Diffuse", stone);
		stoneMat->Set("s_Specular", stoneSpec);
		stoneMat->Set("u_Shininess", 2.0f);
		stoneMat->Set("u_TextureMix", 0.0f); 

		ShaderMaterial::sptr grassMat = ShaderMaterial::Create();
		grassMat->Shader = shader;
		grassMat->Set("s_Diffuse", grass);
		grassMat->Set("s_Specular", noSpec);
		grassMat->Set("u_Shininess", 2.0f);
		grassMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr boxMat = ShaderMaterial::Create();
		boxMat->Shader = shader;
		boxMat->Set("s_Diffuse", box);
		boxMat->Set("s_Specular", boxSpec);
		boxMat->Set("u_Shininess", 8.0f);
		boxMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr simpleFloraMat = ShaderMaterial::Create();
		simpleFloraMat->Shader = shader;
		simpleFloraMat->Set("s_Diffuse", simpleFlora);
		simpleFloraMat->Set("s_Specular", noSpec);
		simpleFloraMat->Set("u_Shininess", 8.0f);
		simpleFloraMat->Set("u_TextureMix", 0.0f);

		//NEW Midterm
		//leaf
		ShaderMaterial::sptr pineLeafMat = ShaderMaterial::Create();
		pineLeafMat->Shader = shader;
		pineLeafMat->Set("s_Diffuse", pineLeaf);
		pineLeafMat->Set("s_Specular", noSpec);
		pineLeafMat->Set("u_Shininess", 2.0f);
		pineLeafMat->Set("u_TextureMix", 0.0f);
		//wood
		ShaderMaterial::sptr pineWoodMat = ShaderMaterial::Create();
		pineWoodMat->Shader = shader;
		pineWoodMat->Set("s_Diffuse", pineWood);
		pineWoodMat->Set("s_Specular", noSpec);
		pineWoodMat->Set("u_Shininess", 1.0f);
		pineWoodMat->Set("u_TextureMix", 0.0f);
		//tank
		//gun
		ShaderMaterial::sptr tracksAndGunMat = ShaderMaterial::Create();
		tracksAndGunMat->Shader = shader;
		tracksAndGunMat->Set("s_Diffuse", trackAndGun);
		tracksAndGunMat->Set("s_Specular", noSpec);
		tracksAndGunMat->Set("u_Shininess", 1.0f);
		tracksAndGunMat->Set("u_TextureMix", 0.0f);
		//1st Color
		ShaderMaterial::sptr primaryMat = ShaderMaterial::Create();
		primaryMat->Shader = shader;
		primaryMat->Set("s_Diffuse", primaryColor);
		primaryMat->Set("s_Specular", noSpec);
		primaryMat->Set("u_Shininess", 1.0f);
		primaryMat->Set("u_TextureMix", 0.0f);
		//2nd Color
		ShaderMaterial::sptr secondaryMat = ShaderMaterial::Create();
		secondaryMat->Shader = shader;
		secondaryMat->Set("s_Diffuse", secondaryColor);
		secondaryMat->Set("s_Specular", noSpec);
		secondaryMat->Set("u_Shininess", 1.0f);
		secondaryMat->Set("u_TextureMix", 0.0f);
		//armor
		ShaderMaterial::sptr armorMat = ShaderMaterial::Create();
		armorMat->Shader = shader;
		armorMat->Set("s_Diffuse", armorPack);
		armorMat->Set("s_Specular", noSpec);
		armorMat->Set("u_Shininess", 1.0f);
		armorMat->Set("u_TextureMix", 0.0f);
		//back Light
		ShaderMaterial::sptr backLightMat = ShaderMaterial::Create();
		backLightMat->Shader = shader;
		backLightMat->Set("s_Diffuse", backLight);
		backLightMat->Set("s_Specular", noSpec);
		backLightMat->Set("u_Shininess", 1.0f);
		backLightMat->Set("u_TextureMix", 0.0f);
		//front Light
		ShaderMaterial::sptr frontLightMat = ShaderMaterial::Create();
		frontLightMat->Shader = shader;
		frontLightMat->Set("s_Diffuse", frontLight);
		frontLightMat->Set("s_Specular", noSpec);
		frontLightMat->Set("u_Shininess", 1.0f);
		frontLightMat->Set("u_TextureMix", 0.0f);
		//front window
		ShaderMaterial::sptr windowMat = ShaderMaterial::Create();
		windowMat->Shader = shader;
		windowMat->Set("s_Diffuse", frontWindow);
		windowMat->Set("s_Specular", noSpec);
		windowMat->Set("u_Shininess", 2.0f);
		windowMat->Set("u_TextureMix", 0.0f);
		//no texture
		ShaderMaterial::sptr noTextureMat = ShaderMaterial::Create();
		noTextureMat->Shader = shader;
		noTextureMat->Set("s_Diffuse", blankTexture);
		noTextureMat->Set("s_Specular", noSpec);
		noTextureMat->Set("u_Shininess", 0.0f);
		noTextureMat->Set("u_TextureMix", 0.0f);
		//Finals ==============================
		//dirt
		ShaderMaterial::sptr terrainDirtMat = ShaderMaterial::Create();
		terrainDirtMat->Shader = shader;
		terrainDirtMat->Set("s_Diffuse", terrainDirt);
		terrainDirtMat->Set("s_Specular", noSpec);
		terrainDirtMat->Set("u_Shininess", 2.0f);
		terrainDirtMat->Set("u_TextureMix", 0.0f);
		//grass
		ShaderMaterial::sptr terraingrassMat = ShaderMaterial::Create();
		terraingrassMat->Shader = shader;
		terraingrassMat->Set("s_Diffuse", terrainGrass);
		terraingrassMat->Set("s_Specular", noSpec);
		terraingrassMat->Set("u_Shininess", 2.0f);
		terraingrassMat->Set("u_TextureMix", 0.0f);
		

		float scenePositionX = 0.0f;
		float scenePositionY = 0.0f;
		float scenePositionZ = 0.0f;

		//Rotation
		float sceneRotationX = 0.0f;
		float sceneRotationY = 0.0f;
		float sceneRotationZ = 0.0f;

		//Scale Factor
		float sceneScaleFactor = 1.0f;

		//Object placements

		GameObject obj1 = scene->CreateEntity("Ground"); 
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plane.obj");
			obj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(grassMat);
			obj1.get<Transform>().SetLocalPosition(0.0f,0.0f, 0.0f);
		}
		//fake rock
		GameObject obj2 = scene->CreateEntity("monkey_quads");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey_quads.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			obj2.get<Transform>().SetLocalPosition(-1.60f, 0.0f, -0.75f);
			obj2.get<Transform>().SetLocalRotation(0.0f, 0.0f, 34.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2);
		}

		

		//tank components
		//Position
		float positionX = 0.0f; // distance from side of monkey (+ left, - right) from start cam pos
		float positionY = 0.0f; // distance from front of monkey (+ further, - behind)
		float positionZ = 0.35f; // height off ground

		//Rotation
		float rotationX = 90.0f;
		float rotationY = 0.0f;
		float rotationZ = 0.0f;

		//Scale Factor
		float scaleFactor = 0.5f;

		//movement points

		//front light
		
		GameObject obj3 = scene->CreateEntity("chaffee_light_f");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/M12_Chaffee_f_lights.obj");
			obj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(frontLightMat); // material set
			obj3.get<Transform>().SetLocalPosition(positionX, positionY, positionZ); //position
			// vertical , rotate around point, horizontal?
			obj3.get<Transform>().SetLocalRotation(rotationX, rotationY, rotationZ); //rotation
			obj3.get<Transform>().SetLocalScale(scaleFactor, scaleFactor, scaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj3);

			//movement path
			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj3);
			// Set up a path for the object to follow

			pathing->Points.push_back({ 0.0f,  1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  2.0f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -2.0f, 0.0f });
			pathing->Speed = 1.0f;
		}
		//track and gun
		GameObject obj4 = scene->CreateEntity("chaffee_tracks");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/M12_Chaffee_black_gun_and_tracks.obj");
			obj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(tracksAndGunMat); // material set
			obj4.get<Transform>().SetLocalPosition(positionX, positionY, positionZ); //position
			// vertical , rotate around point, horizontal?
			obj4.get<Transform>().SetLocalRotation(rotationX, rotationY, rotationZ); //rotation
			obj4.get<Transform>().SetLocalScale(scaleFactor, scaleFactor, scaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj4);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj4);
			pathing->Points.push_back({ 0.0f,  1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  2.0f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -2.0f, 0.0f });
			pathing->Speed = 1.0f;
		}
		//1st Color
		GameObject obj5 = scene->CreateEntity("chaffee_turret");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/M12_Chaffee_1st color_parts.obj");
			obj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(primaryMat); // material set
			obj5.get<Transform>().SetLocalPosition(positionX, positionY, positionZ); //position
			// vertical , rotate around point, horizontal?
			obj5.get<Transform>().SetLocalRotation(rotationX, rotationY, rotationZ); //rotation
			obj5.get<Transform>().SetLocalScale(scaleFactor, scaleFactor, scaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj5);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj5);
			pathing->Points.push_back({ 0.0f,  1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  2.0f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -2.0f, 0.0f });
			pathing->Speed = 1.0f;
		}
		//2nd Color
		GameObject obj6 = scene->CreateEntity("chaffee_main_body");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/M12_Chaffee_2nd color_parts_main_body.obj");
			obj6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(secondaryMat); // material set
			obj6.get<Transform>().SetLocalPosition(positionX, positionY, positionZ); //position
			// vertical , rotate around point, horizontal?
			obj6.get<Transform>().SetLocalRotation(rotationX, rotationY, rotationZ); //rotation
			obj6.get<Transform>().SetLocalScale(scaleFactor, scaleFactor, scaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj6);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj6);
			pathing->Points.push_back({ 0.0f,  1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  2.0f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -2.0f, 0.0f });
			pathing->Speed = 1.0f;
		}
		
		//armor
		GameObject obj7 = scene->CreateEntity("chaffee_armor");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/M12_Chaffee_dark_gray_armaments.obj");
			obj7.emplace<RendererComponent>().SetMesh(vao).SetMaterial(armorMat); // material set
			obj7.get<Transform>().SetLocalPosition(positionX, positionY, positionZ); //position
			// vertical , rotate around point, horizontal?
			obj7.get<Transform>().SetLocalRotation(rotationX, rotationY, rotationZ); //rotation
			obj7.get<Transform>().SetLocalScale(scaleFactor, scaleFactor, scaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj7);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj7);
			pathing->Points.push_back({ 0.0f,  1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  2.0f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -2.0f, 0.0f });
			pathing->Speed = 1.0f;
		}
		
		//back Light
		GameObject obj8 = scene->CreateEntity("chaffee_back_light");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/M12_Chaffee_b_lights.obj");
			obj8.emplace<RendererComponent>().SetMesh(vao).SetMaterial(backLightMat); // material set
			obj8.get<Transform>().SetLocalPosition(positionX, positionY, positionZ); //position
			// vertical , rotate around point, horizontal?
			obj8.get<Transform>().SetLocalRotation(rotationX, rotationY, rotationZ); //rotation
			obj8.get<Transform>().SetLocalScale(scaleFactor, scaleFactor, scaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj8);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj8);
			pathing->Points.push_back({ 0.0f,  1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  2.0f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -2.0f, 0.0f });
			pathing->Speed = 1.0f;
		}
		//back fill
		
		GameObject obj9 = scene->CreateEntity("chaffee_back_fill");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/M12_Chaffee_back_underfill.obj");
			obj9.emplace<RendererComponent>().SetMesh(vao).SetMaterial(secondaryMat); // material set
			obj9.get<Transform>().SetLocalPosition(positionX, positionY, positionZ); //position
			// vertical , rotate around point, horizontal?
			obj9.get<Transform>().SetLocalRotation(rotationX, rotationY, rotationZ); //rotation
			obj9.get<Transform>().SetLocalScale(scaleFactor, scaleFactor, scaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj9);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj9);
			pathing->Points.push_back({ 0.0f,  1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  2.0f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -2.0f, 0.0f });
			pathing->Speed = 1.0f;
		}
		//neck

		GameObject obj10 = scene->CreateEntity("chaffee_neck");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/M12_Chaffee_neck.obj");
			obj10.emplace<RendererComponent>().SetMesh(vao).SetMaterial(armorMat); // material set
			obj10.get<Transform>().SetLocalPosition(positionX, positionY, positionZ); //position
			// vertical , rotate around point, horizontal?
			obj10.get<Transform>().SetLocalRotation(rotationX, rotationY, rotationZ); //rotation
			obj10.get<Transform>().SetLocalScale(scaleFactor, scaleFactor, scaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj10);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj10);
			pathing->Points.push_back({ 0.0f,  1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  2.0f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -2.0f, 0.0f });
			pathing->Speed = 1.0f;
		}
		//windshield
		GameObject obj11 = scene->CreateEntity("chaffee_window");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/M12_Chaffee_windshield.obj");
			obj11.emplace<RendererComponent>().SetMesh(vao).SetMaterial(windowMat); // material set
			obj11.get<Transform>().SetLocalPosition(positionX, positionY, positionZ); //position
			// vertical , rotate around point, horizontal?
			obj11.get<Transform>().SetLocalRotation(rotationX, rotationY, rotationZ); //rotation
			obj11.get<Transform>().SetLocalScale(scaleFactor, scaleFactor, scaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj11);

			// Bind returns a smart pointer to the behaviour that was added
			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj11);
			pathing->Points.push_back({ 0.0f,  1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  2.0f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -1.75f, 0.0f });
			pathing->Points.push_back({ 0.0f,  -2.0f, 0.0f });
			pathing->Speed = 1.0f;
		}
		
		//Tiaga Tree stumps
		//form hexagon
		//left
		GameObject obj12 = scene->CreateEntity("pine_stump_1");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_stump_1.obj");
			obj12.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineWoodMat); // material set
			// vertical , rotate around point, horizontal?
			obj12.get<Transform>().SetLocalPosition(7.0f, 0.0f, 0.0f);
			obj12.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj12);
		}
		GameObject obj13 = scene->CreateEntity("pine_stump_2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_stump_1.obj");
			obj13.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineWoodMat); // material set
			// vertical , rotate around point, horizontal?
			obj13.get<Transform>().SetLocalPosition(3.0f, 6.0f, 0.0f);
			obj13.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj13);
		}
		GameObject obj14 = scene->CreateEntity("pine_stump_3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_stump_1.obj");
			obj14.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineWoodMat); // material set
			// vertical , rotate around point, horizontal?
			obj14.get<Transform>().SetLocalPosition(3.0f, -6.0f, 0.0f);
			obj14.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj14);
		}
		//right
		GameObject obj15 = scene->CreateEntity("pine_stump_4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_stump_1.obj");
			obj15.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineWoodMat); // material set
			// vertical , rotate around point, horizontal?
			obj15.get<Transform>().SetLocalPosition(-7.0f, 0.0f, 0.0f);
			obj15.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj15);
		}
		GameObject obj16 = scene->CreateEntity("pine_stump_5");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_stump_1.obj");
			obj16.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineWoodMat); // material set
			// vertical , rotate around point, horizontal?
			obj16.get<Transform>().SetLocalPosition(-3.0f, 6.0f, 0.0f);
			obj16.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj16);
		}
		GameObject obj17 = scene->CreateEntity("pine_stump_6");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_stump_1.obj");
			obj17.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineWoodMat); // material set
			// vertical , rotate around point, horizontal?
			obj17.get<Transform>().SetLocalPosition(-3.0f, -6.0f, 0.0f);
			obj17.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj17);
		}
		
		//Tiaga Tree leaf
		//form hexagon
		//left
		GameObject obj18 = scene->CreateEntity("pine_leafs_1");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_leaf_1.obj");
			obj18.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineLeafMat); // material set
			// vertical , rotate around point, horizontal?
			obj18.get<Transform>().SetLocalPosition(7.0f, 0.0f, 0.0f);
			obj18.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj18);
		}
		GameObject obj19 = scene->CreateEntity("pine_leafs_2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_leaf_1.obj");
			obj19.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineLeafMat); // material set
			// vertical , rotate around point, horizontal?
			obj19.get<Transform>().SetLocalPosition(3.0f, 6.0f, 0.0f);
			obj19.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj19);
		}
		GameObject obj20 = scene->CreateEntity("pine_leafs_3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_leaf_1.obj");
			obj20.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineLeafMat); // material set
			// vertical , rotate around point, horizontal?
			obj20.get<Transform>().SetLocalPosition(3.0f, -6.0f, 0.0f);
			obj20.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj20);
		}
		//right
		GameObject obj21 = scene->CreateEntity("pine_leafs_4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_leaf_1.obj");
			obj21.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineLeafMat); // material set
			// vertical , rotate around point, horizontal?
			obj21.get<Transform>().SetLocalPosition(-7.0f, 0.0f, 0.0f);
			obj21.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj21);
		}
		GameObject obj22 = scene->CreateEntity("pine_leafs_5");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_leaf_1.obj");
			obj22.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineLeafMat); // material set
			// vertical , rotate around point, horizontal?
			obj22.get<Transform>().SetLocalPosition(-3.0f, 6.0f, 0.0f);
			obj22.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj22);
		}
		GameObject obj23 = scene->CreateEntity("pine_leafs_6");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/taiga_tree_leaf_1.obj");
			obj23.emplace<RendererComponent>().SetMesh(vao).SetMaterial(pineLeafMat); // material set
			// vertical , rotate around point, horizontal?
			obj23.get<Transform>().SetLocalPosition(-3.0f, -6.0f, 0.0f);
			obj23.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj23);
		}
		//fake rock 2 
		GameObject obj24 = scene->CreateEntity("monkey_quads_2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey_quads.obj");
			obj24.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			obj24.get<Transform>().SetLocalPosition(1.60f, 3.0f, -0.75f);
			obj24.get<Transform>().SetLocalRotation(0.0f, 0.0f, -50.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj24);
		}
		
		/*
		//Finals
		GameObject obj3 = scene->CreateEntity("Dirt Terrain Ground");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/dirt.obj");
		obj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(terrainDirtMat); // material set
		// vertical , rotate around point, horizontal?
		obj3.get<Transform>().SetLocalPosition(scenePositionX, scenePositionY, scenePositionZ);
		obj3.get<Transform>().SetLocalRotation(sceneRotationX, sceneRotationY, sceneRotationZ);
		obj3.get<Transform>().SetLocalScale(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);//scale down
		BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj3);
		}

		GameObject obj4 = scene->CreateEntity("Grass Terrain Ground");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/grass.obj");
			obj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(terraingrassMat); // material set
			// vertical , rotate around point, horizontal?
			obj4.get<Transform>().SetLocalPosition(scenePositionX, scenePositionY, scenePositionZ);
			obj4.get<Transform>().SetLocalRotation(sceneRotationX, sceneRotationY, sceneRotationZ);
			obj4.get<Transform>().SetLocalScale(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj4);
		}

		GameObject obj5 = scene->CreateEntity("Car Body");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/car body.obj");
			obj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(terraingrassMat); // material set
			// vertical , rotate around point, horizontal?
			obj5.get<Transform>().SetLocalPosition(scenePositionX, scenePositionY, scenePositionZ);
			obj5.get<Transform>().SetLocalRotation(sceneRotationX, sceneRotationY, sceneRotationZ);
			obj5.get<Transform>().SetLocalScale(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);//scale down
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj5);
		}

		*/

		

		//generation
		//trees
		std::vector<glm::vec2> allAvoidAreasFrom = { glm::vec2(-8.0f, -8.0f) }; //back tree clear param
		std::vector<glm::vec2> allAvoidAreasTo = { glm::vec2(8.0f, 8.0f) }; // front tree clear param
		//rocks
		std::vector<glm::vec2> rockAvoidAreasFrom = { glm::vec2(-3.0f, -3.0f), glm::vec2(-19.0f, -19.0f), glm::vec2(5.0f, -19.0f),
														glm::vec2(-19.0f, 5.0f), glm::vec2(-19.0f, -19.0f) };
		std::vector<glm::vec2> rockAvoidAreasTo = { glm::vec2(3.0f, 3.0f), glm::vec2(19.0f, -5.0f), glm::vec2(19.0f, 19.0f),
														glm::vec2(19.0f, 19.0f), glm::vec2(-5.0f, 19.0f) };
		//spawn in
		glm::vec2 spawnFromHere = glm::vec2(-19.0f, -19.0f);
		glm::vec2 spawnToHere = glm::vec2(19.0f, 19.0f);

		EnvironmentGenerator::AddObjectToGeneration("models/simplePine.obj", simpleFloraMat, 150,
			spawnFromHere, spawnToHere, allAvoidAreasFrom, allAvoidAreasTo);
		EnvironmentGenerator::AddObjectToGeneration("models/simpleTree.obj", simpleFloraMat, 150,
			spawnFromHere, spawnToHere, allAvoidAreasFrom, allAvoidAreasTo);
		EnvironmentGenerator::AddObjectToGeneration("models/simpleRock.obj", simpleFloraMat, 40,
			spawnFromHere, spawnToHere, rockAvoidAreasFrom, rockAvoidAreasTo);
		EnvironmentGenerator::GenerateEnvironment();

		

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);

		GameObject framebufferObject = scene->CreateEntity("Basic Effect");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
		}
		effects.push_back(sepiaEffect);

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);
		
		GameObject colorCorrectEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			colorCorrectEffect = &colorCorrectEffectObject.emplace<ColorCorrectEffect>();
			colorCorrectEffect->Init(width, height);
		}
		effects.push_back(colorCorrectEffect);
		//bloom
		GameObject bloomEffectObject = scene->CreateEntity("Bloom Effect");
		{
			bloomEffect = &bloomEffectObject.emplace<BloomEffect>();
			bloomEffect->Init(width, height);
		}
		effects.push_back(bloomEffect);
		//depth effect
		GameObject depthEffectObject = scene->CreateEntity("Depth Effect");
		{
			depthEffect = &depthEffectObject.emplace<DepthEffect>();
			depthEffect->Init(width, height);
		}
		effects.push_back(depthEffect);

		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			controllables.push_back(obj2);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			// Clear the screen
			basicEffect->Clear();
			/*greyscaleEffect->Clear();
			sepiaEffect->Clear();*/
			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear();
			}


			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});
			
			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

		
			

			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;
				
				return false;
			});



			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;
			

			basicEffect->BindBuffer(0);

			// Iterate over the render group components and draw them
			renderGroup.each( [&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				

				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
			});

			// Render the mesh //midterm
			if (isTextureEnabled == 0) { //off
			// Start by assuming no shader or material is applied

				grassMat->Set("s_Diffuse", blankTexture);
				stoneMat->Set("s_Diffuse", blankTexture);
				boxMat->Set("s_Diffuse", blankTexture);
				windowMat->Set("s_Diffuse", blankTexture);
				//////
				simpleFloraMat->Set("s_Diffuse", blankTexture);
				pineLeafMat->Set("s_Diffuse", blankTexture);
				pineWoodMat->Set("s_Diffuse", blankTexture);
				tracksAndGunMat->Set("s_Diffuse", blankTexture);
				primaryMat->Set("s_Diffuse", blankTexture);
				secondaryMat->Set("s_Diffuse", blankTexture);
				armorMat->Set("s_Diffuse", blankTexture);
				backLightMat->Set("s_Diffuse", blankTexture);
				frontLightMat->Set("s_Diffuse", blankTexture);
				windowMat->Set("s_Diffuse", blankTexture);
			}
			else { //on
				stoneMat->Set("s_Diffuse", stone);
				grassMat->Set("s_Diffuse", grass);
				boxMat->Set("s_Diffuse", box);
				//////
				simpleFloraMat->Set("s_Diffuse", simpleFlora);
				pineLeafMat->Set("s_Diffuse", pineLeaf);
				pineWoodMat->Set("s_Diffuse", pineWood);
				tracksAndGunMat->Set("s_Diffuse", trackAndGun);
				primaryMat->Set("s_Diffuse", primaryColor);
				secondaryMat->Set("s_Diffuse", secondaryColor);
				armorMat->Set("s_Diffuse", armorPack);
				backLightMat->Set("s_Diffuse", backLight);
				frontLightMat->Set("s_Diffuse", frontLight);
				windowMat->Set("s_Diffuse", frontWindow);
				//////

			}

			basicEffect->UnbindBuffer();

			effects[activeEffect]->ApplyEffect(basicEffect);
			
			effects[activeEffect]->DrawToScreen();
			
			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		//Clean up the environment generator so we can release references
		EnvironmentGenerator::CleanUpPointers();
		BackendHandler::ShutdownImGui();
	}	

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}