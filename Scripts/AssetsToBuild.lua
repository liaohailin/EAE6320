--[[
	This is an my assets list file
]]

return
{
	-- This table contains a hypothetical list of assets file name
	-- (using an array)
	generic =
	{
		builder = "GenericBuilder.exe",
		assets =
		{
			"braceMat.mat",
			"cementWallMat.mat",
			"railingMat.mat",
			"otherObjectMat.mat",
			"wallMat.mat",
			"floorMat.mat",
			"catMat.mat",
			"collisionMat.mat",
			"redMat.mat",
			"blueMat.mat",
			"dogMat.mat",
			"winn-up.wav",
			"gotFlag.wav",
			"backgroundMusic.wav",
			"walk.wav",
		},
	},
	vertexShader =
	{
		builder = "VertexShaderBuilder.exe",
		assets =
		{
			"vertexShader.hlsl",
			"spriteVertexShader.hlsl",
			"octreeCollisionDataVertexShader.hlsl",
		},
	},
	fragmentShader =
	{
		builder = "FragmentShaderBuilder.exe",
		assets =
		{
			"fragmentShader.hlsl",
			"spriteFragmentShader.hlsl",
			"octreeCollisionDataFragmentShader.hlsl",
		},
	},
	meshFiles =
	{
		builder = "MeshBuilder.exe",
		assets =
		{
			"braceMesh.mesh",
			"cementWallMesh.mesh",
			"otherObjectMesh.mesh",
			"wallMesh.mesh",
			"railingMesh.mesh",
			"floorMesh.mesh",
			"debugBoxMesh.mesh",
			"debugSphereMesh.mesh",
			"sceneCollisionInfo.mesh",
			"catMesh.mesh",
			"flagMesh.mesh",
			"dogMesh.mesh",
		},
	},
	collisionDataFiles =
	{
		builder = "CollisionDataBuilder.exe",
		assets =
		{
			"octreeCollisionInfo.col",
		},
	},
	textures =
	{
		builder = "TextureBuilder.exe",
		assets =
		{
			"floor.png",
			"cement_wall.png",
			"metal_brace.png",
			"railing.png",
			"wall.png",
			"cat.tga",
			"collisionTexture.png",
			"redMaterial.png",
			"blueMaterial.png",
			"dog.tga",
		},
	},
}