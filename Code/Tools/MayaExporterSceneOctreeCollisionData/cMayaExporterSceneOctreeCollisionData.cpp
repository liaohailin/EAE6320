// Header Files
//=============

#include "cMayaExporterSceneOctreeCollisionData.h"


#include <math.h>
#include <stdio.h>

#include <cstdint>
#include <fstream>
#include <map>
#include <maya/MDagPath.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItDag.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItSelectionList.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MSelectionList.h>
#include <sstream>
#include <string>

// Vertex Definition
//==================
#define OCTREE_DEPTH 3
namespace
{
	struct sVertex
	{
		// Position
		float x, y, z;
		// Normal
		float nx, ny, nz;
		// Tangent
		float tx, ty, tz;
		// Bitangent
		float btx, bty, btz;
		// Texture coordinates
		float u, v;
		// Color
		// (Note that each color channel is a float and [0,1], _not_ a uint8_t [0,255]!)
		float r, g, b, a;

		sVertex( const MPoint& i_position, const MFloatVector& i_normal,
			const MFloatVector& i_tangent, const MFloatVector& i_bitangent,
			const float i_texcoordU, const float i_texcoordV,
			const MColor& i_vertexColor )
			:
			x( static_cast<float>( i_position.x ) ), y( static_cast<float>( i_position.y ) ), z( static_cast<float>( i_position.z ) ),
			nx( i_normal.x ), ny( i_normal.y ), nz( i_normal.z ),
			tx( i_tangent.x ), ty( i_tangent.y ), tz( i_tangent.z ),
			btx( i_bitangent.x ), bty( i_bitangent.y ), btz( i_bitangent.z ),
			u( i_texcoordU ), v( i_texcoordV ),
			r( i_vertexColor.r ), g( i_vertexColor.g ), b( i_vertexColor.b ), a( i_vertexColor.a )
		{

		}
	};

	struct BoundingBox
	{
		float halfWidth;
		float centerX, centerY, centerZ;
	};

	struct OctreeNode
	{
		BoundingBox boundingBox;
		std::vector<unsigned int> triangleIndexList;
		unsigned int depth;
		OctreeNode* children[8];
	};

}

// Helper Function Declarations
//=============================

namespace
{
	std::string CreateUniqueVertexKey( const int i_positionIndex, const int i_normalIndex,
		const int i_tangentIndex, const int i_texcoordIndex, const int i_vertexColorIndex );
	MStatus ProcessAllMeshes( std::vector<const sVertex>& o_vertexBuffer, std::vector<unsigned int>& o_indexBuffer );
	MStatus ProcessSelectedMeshes( std::vector<const sVertex>& o_vertexBuffer, std::vector<unsigned int>& o_indexBuffer );
	MStatus ProcessSingleMesh( const MFnMesh& i_mesh,
		std::vector<const sVertex>& o_vertexBuffer, std::vector<unsigned int>& o_indexBuffer );
	MStatus FillVertexAndIndexBuffer( const MFnMesh& i_mesh,
		std::vector<const sVertex>& o_vertexBuffer, std::vector<unsigned int>& o_indexBuffer );
	MStatus WriteMeshToFile( const MString& i_fileName, std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer );
	MStatus WriteCollisionDataToFile(const MString& i_fileName, OctreeNode* i_octree, std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer);
	void WriteOctreeOut(OctreeNode* i_currentOctreeNode, std::ofstream& i_writer, char*& i_nodeTrack);
	void WriteTriangleInfo(std::ofstream& i_writer, std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer);
	OctreeNode* BuildOctreeFromData(std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer, //mesh info
		std::vector<unsigned int>& i_triangleIndexList,//partitional box triangle info
		BoundingBox& i_boundingBox,//bounding box for the current box
		unsigned int i_depth);//depth of the current octree
	int TriangleBoxOverLap(std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer,
		unsigned int i_triangleIndex,
		BoundingBox& i_boundingBox);
	int planeBoxOverlap(float normal[3], float vert[3], float maxbox[3]);
}

// Inherited Interface
//====================

MStatus eae6320::cMayaExporterSceneOctreeCollisionData::writer(const MFileObject& i_file, const MString& i_options, FileAccessMode i_mode)
{
	// Gather the vertex and index buffer information
	std::vector<const sVertex> vertexBuffer;
	std::vector<unsigned int> indexBuffer;
	{
		// The user decides whether to export the entire scene or just a selection
		if ( i_mode == MPxFileTranslator::kExportAccessMode )
		{
			const MStatus status = ProcessAllMeshes( vertexBuffer, indexBuffer );
			if ( !status )
			{
				return status;
			}
		}
		else if ( i_mode == MPxFileTranslator::kExportActiveAccessMode )
		{
			const MStatus status = ProcessSelectedMeshes( vertexBuffer, indexBuffer );
			if ( !status )
			{
				return status;
			}
		}
		else
		{
			MGlobal::displayError( "Unexpected file access mode" );
			return MStatus::kFailure;
		}
	}

	//build octree
	OctreeNode* octree; 
	{
		//get scene bounding box
		BoundingBox boundingBox;
		float maxX = vertexBuffer[0].x, minX = vertexBuffer[0].x;
		float maxY = vertexBuffer[0].y, minY = vertexBuffer[0].y;
		float maxZ = vertexBuffer[0].z, minZ = vertexBuffer[0].z;

		for (unsigned int i = 0; i < vertexBuffer.size(); i++)
		{
			if (vertexBuffer[i].x>maxX)maxX = vertexBuffer[i].x;
			if (vertexBuffer[i].x<minX)minX = vertexBuffer[i].x;

			if (vertexBuffer[i].y>maxY)maxY = vertexBuffer[i].y;
			if (vertexBuffer[i].y<minY)minY = vertexBuffer[i].y;

			if (vertexBuffer[i].z>maxZ)maxZ = vertexBuffer[i].z;
			if (vertexBuffer[i].z<minZ)minZ = vertexBuffer[i].z;
		}
		float halfWidth = (maxX - minX)/2;
		if (halfWidth < (maxY - minY) / 2)halfWidth = (maxY - minY) / 2;
		if (halfWidth < (maxZ - minZ) / 2)halfWidth = (maxZ - minZ) / 2;

		float centerX = (maxX + minX) / 2;
		float centerY = (maxY + minY) / 2;
		float centerZ = (maxZ + minZ) / 2;

		boundingBox.centerX = centerX;
		boundingBox.centerY = centerY;
		boundingBox.centerZ = centerZ;
		boundingBox.halfWidth = halfWidth;
		//initialize triangle list
		std::vector<unsigned int>triangleIndexList;
		triangleIndexList.clear();
		for (unsigned int i = 0; i < indexBuffer.size()/3; i++)
		{
			triangleIndexList.push_back(i);
		}
		octree = BuildOctreeFromData(vertexBuffer,indexBuffer,triangleIndexList,boundingBox,0);
	}

	// Write the mesh to the requested file
	{
		const MString filePath = i_file.fullName();
		return WriteCollisionDataToFile(filePath, octree,vertexBuffer,indexBuffer);
	}
}

// Helper Function Definitions
//============================

namespace
{
	std::string CreateUniqueVertexKey( const int i_positionIndex, const int i_normalIndex,
		const int i_tangentIndex, const int i_texcoordIndex, const int i_vertexColorIndex )
	{
		std::ostringstream vertexKey;
		vertexKey << i_positionIndex << "_" << i_normalIndex << "_"
			<< i_tangentIndex << "_" << i_texcoordIndex << "_" << i_vertexColorIndex;
		return vertexKey.str();
	}

	MStatus ProcessAllMeshes( std::vector<const sVertex>& o_vertexBuffer, std::vector<unsigned int>& o_indexBuffer )
	{
		for ( MItDag i( MItDag::kDepthFirst, MFn::kMesh ); !i.isDone(); i.next() )
		{
			MFnMesh mesh( i.item() );
			if ( !ProcessSingleMesh( mesh, o_vertexBuffer, o_indexBuffer ) )
			{
				return MStatus::kFailure;
			}
		}

		return MStatus::kSuccess;
	}

	MStatus ProcessSelectedMeshes( std::vector<const sVertex>& o_vertexBuffer, std::vector<unsigned int>& o_indexBuffer )
	{
		// Iterate through each selected mesh
		MSelectionList selectionList;
		MStatus status = MGlobal::getActiveSelectionList( selectionList );
		if ( status )
		{
			for ( MItSelectionList i( selectionList, MFn::kMesh ); !i.isDone(); i.next() )
			{
				MDagPath dagPath;
				i.getDagPath( dagPath );
				MFnMesh mesh( dagPath );
				if ( !ProcessSingleMesh( mesh, o_vertexBuffer, o_indexBuffer ) )
				{
					return MStatus::kFailure;
				}
			}
		}
		else
		{
			MGlobal::displayError( MString( "Failed to get active selection list: " ) + status.errorString() );
			return MStatus::kFailure;
		}

		return MStatus::kSuccess;
	}

	MStatus ProcessSingleMesh( const MFnMesh& i_mesh,
		std::vector<const sVertex>& o_vertexBuffer, std::vector<unsigned int>& o_indexBuffer )
	{
		if ( i_mesh.isIntermediateObject() )
		{
			return MStatus::kSuccess;
		}

		return FillVertexAndIndexBuffer( i_mesh, o_vertexBuffer, o_indexBuffer );
	}

	MStatus FillVertexAndIndexBuffer( const MFnMesh& i_mesh,
		std::vector<const sVertex>& o_vertexBuffer, std::vector<unsigned int>& o_indexBuffer )
	{
		MStatus status;

		// Get a list of the positions
		MPointArray positions;
		{
			status = i_mesh.getPoints( positions );
			if ( !status )
			{
				MGlobal::displayError( status.errorString() );
				return status;
			}
		}

		// Get a list of the normals
		MFloatVectorArray normals;
		{
			status = i_mesh.getNormals( normals );
			if ( !status )
			{
				MGlobal::displayError( status.errorString() );
				return status;
			}
		}

		// Get a list of tangents
		MFloatVectorArray tangents;
		{
			status = i_mesh.getTangents( tangents );
			if ( !status )
			{
				MGlobal::displayError( status.errorString() );
				return status;
			}
		}

		// Get a list of bitangents
		MFloatVectorArray bitangents;
		{
			status = i_mesh.getBinormals( bitangents );
			if ( !status )
			{
				MGlobal::displayError( status.errorString() );
				return status;
			}
		}

		// Get a list of the texture coordinates
		MFloatArray texcoordUs, texcoordVs;
		{
			status = i_mesh.getUVs( texcoordUs, texcoordVs );
			if ( !status )
			{
				MGlobal::displayError( status.errorString() );
				return status;
			}
		}

		// Get a list of the vertex colors
		MColorArray vertexColors;
		{
			int colorSetCount = i_mesh.numColorSets();
			if ( colorSetCount > 0 )
			{
				MString* useDefaultColorSet = NULL;
				MColor defaultColor( 1.0f, 1.0f, 1.0f, 1.0f );
				status = i_mesh.getColors( vertexColors, useDefaultColorSet, &defaultColor );
				if ( !status )
				{
					MGlobal::displayError( status.errorString() );
					return status;
				}
			}
		}

		// Gather vertex and triangle information
		std::map<const std::string, const sVertex> uniqueVertices;
		std::vector<const std::string> triangles;
		{
			MPointArray trianglePositions;
			MIntArray positionIndices;
			for ( MItMeshPolygon i( i_mesh.object() ); !i.isDone(); i.next() )
			{
				if ( i.hasValidTriangulation() )
				{
					// Store information for each vertex in the polygon
					std::map<int, const std::string> indexToKeyMap;
					{
						MIntArray vertices;
						status = i.getVertices( vertices );
						if ( status )
						{
							for ( unsigned int j = 0; j < vertices.length(); ++j )
							{
								const int positionIndex = vertices[j];
								const int normalIndex = i.normalIndex( j );
								const int tangentIndex = i.tangentIndex( j );
								int texcoordIndex;
								{
									status = i.getUVIndex( j, texcoordIndex );
									if ( !status )
									{
										MGlobal::displayError( status.errorString() );
										return status;
									}
								}
								int vertexColorIndex = -1;
								MColor vertexColor( 1.0f, 1.0f, 1.0f, 1.0f );
								{
									int colorSetCount = i_mesh.numColorSets();
									if ( colorSetCount > 0 )
									{
										status = i.getColorIndex( j, vertexColorIndex );
										if ( status )
										{
											vertexColor = vertexColors[vertexColorIndex];
										}
										else
										{
											MGlobal::displayError( status.errorString() );
											return status;
										}
									}
								}
								const std::string vertexKey = CreateUniqueVertexKey( positionIndex, normalIndex,
									tangentIndex, texcoordIndex, vertexColorIndex );
								indexToKeyMap.insert( std::make_pair( positionIndex, vertexKey ) );
								uniqueVertices.insert( std::make_pair( vertexKey,
									sVertex( positions[positionIndex], normals[normalIndex],
										tangents[tangentIndex], bitangents[tangentIndex],
										texcoordUs[texcoordIndex], texcoordVs[texcoordIndex],
										vertexColor ) ) );
							}
						}
						else
						{
							MGlobal::displayError( status.errorString() );
							return status;
						}
					}
					// Store information for each individual triangle in the polygon
					{
						int triangleCount = 0;
						i.numTriangles( triangleCount );
						for ( int j = 0; j < triangleCount; ++j )
						{
							i.getTriangle( j, trianglePositions, positionIndices );
							for ( unsigned int k = 0; k < positionIndices.length(); ++k )
							{
								const int positionIndex = positionIndices[k];
								std::map<int, const std::string>::iterator mapLookUp = indexToKeyMap.find( positionIndex );
								if ( mapLookUp != indexToKeyMap.end() )
								{
									const std::string& vertexKey = mapLookUp->second;
									triangles.push_back( vertexKey );
								}
								else
								{
									MGlobal::displayError( "A triangle gave a different vertex index than the polygon gave" );
									return MStatus::kFailure;
								}
							}
						
						}
					}
				}
				else
				{
					MGlobal::displayError( "This mesh has an invalid triangulation" );
					return MStatus::kFailure;
				}
			}
		}

		// Convert the triangle information to vertex and index buffers
		o_vertexBuffer.clear();
		o_indexBuffer.clear();
		o_indexBuffer.resize( triangles.size() );
		{
			std::map<const std::string, unsigned int> keyToIndexMap;
			for ( std::map<const std::string, const sVertex>::iterator i = uniqueVertices.begin(); i != uniqueVertices.end(); ++i )
			{
				keyToIndexMap.insert( std::make_pair( i->first, static_cast<unsigned int>( o_vertexBuffer.size() ) ) );
				o_vertexBuffer.push_back( i->second );
			}
			for ( size_t i = 0; i < triangles.size(); ++i )
			{
				const std::string& key = triangles[i];
				unsigned int index = keyToIndexMap.find( key )->second;
				o_indexBuffer[i] = index;
			}
		}

		return MStatus::kSuccess;
	}

	OctreeNode* BuildOctreeFromData(std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer, 
		std::vector<unsigned int>& i_triangleIndexList,
		BoundingBox& i_boundingBox,
		unsigned int i_depth)
	{
		//if satistfy depth condition stop divide box and return null
		if (i_depth > OCTREE_DEPTH)
			return NULL;

		//else continue dividing octree
		OctreeNode* octree = new OctreeNode;
		octree->boundingBox.halfWidth = i_boundingBox.halfWidth;
		octree->boundingBox.centerX = i_boundingBox.centerX;
		octree->boundingBox.centerY = i_boundingBox.centerY;
		octree->boundingBox.centerZ = i_boundingBox.centerZ;
		octree->depth = i_depth;
		octree->triangleIndexList = i_triangleIndexList;

		//build eight bounding box according to the current bounding box
		BoundingBox eightBoundingBoxes[8];
		{
			for (unsigned int i = 0; i < 8; i++)
			{
				eightBoundingBoxes[i].halfWidth = i_boundingBox.halfWidth / 2;

				//upper boxes
				if (i < 4)
				{
					eightBoundingBoxes[i].centerY = i_boundingBox.centerY + i_boundingBox.halfWidth / 2;
				}
				else//lower 4 boxes
				{
					eightBoundingBoxes[i].centerY = i_boundingBox.centerY - i_boundingBox.halfWidth / 2;
				}

				//back boxes
				if (i == 0 || i == 1 || i == 4 || i == 5)
				{
					eightBoundingBoxes[i].centerZ = i_boundingBox.centerZ + i_boundingBox.halfWidth / 2;
				}
				else//front boxes
				{
					eightBoundingBoxes[i].centerZ = i_boundingBox.centerZ - i_boundingBox.halfWidth / 2;
				}

				//right boxes
				if (i == 1 || i == 3 || i == 5 || i == 7)
				{
					eightBoundingBoxes[i].centerX = i_boundingBox.centerX + i_boundingBox.halfWidth / 2;
				}
				else//left boxes
				{
					eightBoundingBoxes[i].centerX = i_boundingBox.centerX - i_boundingBox.halfWidth / 2;
				}

			}
		}

		std::vector<unsigned int> triangleIndexLists[8];

		//loop throught all the current triangle list to check the intersection of the triangle and the box
		for (unsigned int i = 0; i < i_triangleIndexList.size(); i++)
		{
			for (unsigned int j = 0; j < 8; j++)
			{
				if (TriangleBoxOverLap(i_vertexBuffer, i_indexBuffer, i_triangleIndexList[i], eightBoundingBoxes[j]))
				{
					triangleIndexLists[j].push_back(i_triangleIndexList[i]);
				}
			}
			
		}

		//if some box have triangles in it, do the recursive call
		for (unsigned int i = 0; i < 8; i++)
		{
			if (triangleIndexLists[i].size()>0)
			{
				octree->children[i] = BuildOctreeFromData(i_vertexBuffer, i_indexBuffer, triangleIndexLists[i], eightBoundingBoxes[i], i_depth + 1);
			}
			else
			{
				octree->children[i] = NULL;
			}
		}
		return octree;
	}
	void WriteOctreeOut(OctreeNode* i_currentOctreeNode, std::ofstream& i_writer, char*& i_nodeTrack)
	{
		int blank=0;
		for (unsigned int i = 0; i < 8; i++)
		{
			if (i_currentOctreeNode->children[i] != NULL)
			{
				i_nodeTrack[i_currentOctreeNode->depth] = (char)(i + 48);

				WriteOctreeOut(i_currentOctreeNode->children[i], i_writer, i_nodeTrack);

				for (unsigned int cleanIterator = i_currentOctreeNode->depth; cleanIterator < OCTREE_DEPTH; cleanIterator++)
				{
					i_nodeTrack[cleanIterator] = (char)(8+48);
				}
			}
			else blank++;
		}
		if (blank == 8) 
		{
			i_writer << "{\n";
			//write node track
			i_writer << "nodeTrack={";
			for (unsigned int i = 0; i < OCTREE_DEPTH; i++)
			{
				i_writer << i_nodeTrack[i] << ",";
			}
			i_writer << "},\n";
			//is collision data
			i_writer << "isCollision=true,\n";
			//write depth
			i_writer << "depth=" << i_currentOctreeNode->depth << ",\n";
			//bounding box info
			i_writer << "halfWidth=" << i_currentOctreeNode->boundingBox.halfWidth << ",\n";
			i_writer << "center={" << i_currentOctreeNode->boundingBox.centerX << "," << i_currentOctreeNode->boundingBox.centerY << "," << -i_currentOctreeNode->boundingBox.centerZ << "},\n";
			
			//write triangle in list number
			i_writer << "triangleNum=" << i_currentOctreeNode->triangleIndexList.size()<< ",\n";
			//write triangle list
			i_writer << "triangleList={";
			for (unsigned int i = 0; i < i_currentOctreeNode->triangleIndexList.size(); i++)
			{
				i_writer << i_currentOctreeNode->triangleIndexList[i] << ",";
			}
			i_writer << "},\n";


			i_writer << "},\n";
		}
		else
		{
			i_writer << "{\n";
			//write node track
			i_writer << "nodeTrack={";
			for (unsigned int i = 0; i < OCTREE_DEPTH; i++)
			{
				i_writer << i_nodeTrack[i]<<",";
			}
			i_writer << "},\n";
			//is collision data
			i_writer << "isCollision=false,\n";
			//write depth
			i_writer << "depth=" << i_currentOctreeNode->depth << ",\n";
			//bounding box info
			i_writer << "halfWidth=" << i_currentOctreeNode->boundingBox.halfWidth << ",\n";
			i_writer << "center={" << i_currentOctreeNode->boundingBox.centerX << "," << i_currentOctreeNode->boundingBox.centerY << "," << -i_currentOctreeNode->boundingBox.centerZ << "},\n";

			//write triangle in list number
			i_writer << "triangleNum=0,\n";
			//write triangle list
			i_writer << "triangleList={},\n";


			i_writer << "},\n";
		}
	}
	void WriteTriangleInfo(std::ofstream& i_writer, std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer)
	{
		//write vertex
		i_writer << "vertices=\n{\n";
		for (size_t i = 0; i < i_vertexBuffer.size(); i++)
		{
			i_writer << i_vertexBuffer[i].x << "," << i_vertexBuffer[i].y << "," << -i_vertexBuffer[i].z << ",\n";
		}
		i_writer << "},\n";

		//write indices
		i_writer << "indices=\n{\n";
		for (size_t i = 0; i < i_indexBuffer.size(); i++)
		{
			//write indices
			if (i % 3 == 1)
				i_writer << i_indexBuffer[i + 1] << ",";
			else if (i % 3 == 2)
				i_writer << i_indexBuffer[i - 1] << ",";
			else i_writer << i_indexBuffer[i] << ",";
		}
		i_writer << "\n},\n";
			
	}

	MStatus WriteCollisionDataToFile(const MString& i_fileName, OctreeNode* i_octree, std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer)
	{
		// Maya's coordinate system is different than the default Direct3D behavior
		// (it is right handed and UVs have (0,0) at the lower left corner).
		// You can deal with these differences however you want,
		// but if you want to convert everything in the exported file
		// the following are probably the most natural changes to make:
		//	* POSITION	-> x, y, -z
		//	* NORMAL	-> nx, ny, -z
		//	* TANGENT	-> tx, ty, -tz
		//	* BITANGENT	-> -btx, -bty, btz
		//	* TEXCOORD	-> u, 1 - v
		
		//detect if octree is NULL
		if (i_octree == NULL)
		{
			MGlobal::displayError(MString("Octree is empty."));
			return MStatus::kFailure;
		}

		std::ofstream fout(i_fileName.asChar());
		if (fout.is_open())
		{
			// Open table
			fout << "return\n{\n";
			{
				//write the number of vertex
				fout << "vertexNum="<< i_vertexBuffer.size()*3<< ",\n";
				//write the number of indice
				fout << "indiceNum="<< i_indexBuffer.size()<< ",\n";
				//write triangle vertices and indices
				WriteTriangleInfo(fout, i_vertexBuffer, i_indexBuffer);

				//write octree
				fout << "octreeDepth=" << OCTREE_DEPTH << ",\n";
				fout << "octrees=\n{\n";
				if (i_octree != NULL)
				{
					char* groupTrack=new char[OCTREE_DEPTH];
					for (unsigned int i = 0; i < OCTREE_DEPTH; i++)
					{
						groupTrack[i] = 'n';
					}
					WriteOctreeOut(i_octree, fout,groupTrack);
				}
				fout << "},\n";

			}
			// Close table
			fout << "}\n";
			fout.close();

			return MStatus::kSuccess;
		}
		else
		{
			MGlobal::displayError(MString("Couldn't open ") + i_fileName + " for writing");
			return MStatus::kFailure;
		}
	}
	MStatus WriteMeshToFile( const MString& i_fileName, std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer )
	{
		// Maya's coordinate system is different than the default Direct3D behavior
		// (it is right handed and UVs have (0,0) at the lower left corner).
		// You can deal with these differences however you want,
		// but if you want to convert everything in the exported file
		// the following are probably the most natural changes to make:
		//	* POSITION	-> x, y, -z
		//	* NORMAL	-> nx, ny, -z
		//	* TANGENT	-> tx, ty, -tz
		//	* BITANGENT	-> -btx, -bty, btz
		//	* TEXCOORD	-> u, 1 - v

		std::ofstream fout( i_fileName.asChar() );
		if ( fout.is_open() )
		{
			// Open table
			fout << "return\n{\n";
			{
				//write the number of vertex
				fout << "vertexNum=";
				fout << i_vertexBuffer.size(); 
				fout << ",\n";
				//write the number of indice
				fout << "indiceNum=";
				fout << i_indexBuffer.size();
				fout << ",\n";


				//write vertex
				fout << "vertices=\n{\n";
				for (size_t i = 0; i < i_vertexBuffer.size(); i++)
				{
					fout << "{\n";

					//write position
					fout << "position={"<< i_vertexBuffer[i].x << "," << i_vertexBuffer[i].y << "," << -i_vertexBuffer[i].z << "},\n";
					//normal
					fout << "normal={" << i_vertexBuffer[i].nx << "," << i_vertexBuffer[i].ny << "," << -i_vertexBuffer[i].nz << "},\n";
					//write color
					fout << "color={" << i_vertexBuffer[i].r << "," << i_vertexBuffer[i].g << "," << i_vertexBuffer[i].b << "},\n";
					//write texcoord
					fout << "texcoord={" << i_vertexBuffer[i].u << "," << 1-i_vertexBuffer[i].v << "," << "},\n";

					fout << "},\n";
				}
				fout << "},\n";

				//write indices
				fout << "indices=\n{\n";
				for (size_t i = 0; i < i_indexBuffer.size(); i++)
				{
					//write indices
					if (i%3==1)
						fout << i_indexBuffer[i+1] << ",";
					else if (i%3==2)
						fout << i_indexBuffer[i - 1] << ",";
					else fout << i_indexBuffer[i] << ",";
				}
				fout << "\n},\n";
			}
			// Close table
			fout << "}\n";
			fout.close();

			return MStatus::kSuccess;
		}
		else
		{
			MGlobal::displayError( MString( "Couldn't open " ) + i_fileName + " for writing" );
			return MStatus::kFailure;
		}
	}

	/********************************************************/

	/* AABB-triangle overlap test code                      */

	/* by Tomas Akenine-Möller                              */

	/* Function: int triBoxOverlap(float boxcenter[3],      */

	/*          float boxhalfsize[3],float triverts[3][3]); */

	/* History:                                             */

	/*   2001-03-05: released the code in its first version */

	/*   2001-06-18: changed the order of the tests, faster */

	/*                                                      */

	/* Acknowledgement: Many thanks to Pierre Terdiman for  */

	/* suggestions and discussions on how to optimize code. */

	/* Thanks to David Hunt for finding a ">="-bug!         */

	/********************************************************/
	int planeBoxOverlap(float normal[3], float vert[3], float maxbox[3])	// -NJMP-
	{

		int q;

		float vmin[3], vmax[3], v;

		for (q = 0; q <= 2; q++)

		{

			v = vert[q];					// -NJMP-

			if (normal[q]>0.0f)

			{

				vmin[q] = -maxbox[q] - v;	// -NJMP-

				vmax[q] = maxbox[q] - v;	// -NJMP-

			}

			else

			{

				vmin[q] = maxbox[q] - v;	// -NJMP-

				vmax[q] = -maxbox[q] - v;	// -NJMP-

			}

		}

		if ((normal[0] * vmin[0] + normal[1] * vmin[1] + normal[2] * vmin[2])>0.0f) return 0;	// -NJMP-

		if ((normal[0] * vmax[0] + normal[1] * vmax[1] + normal[2] * vmax[2]) >= 0.0f) return 1;	// -NJMP-


		return 0;

	}

	int TriangleBoxOverLap(std::vector<const sVertex>& i_vertexBuffer, std::vector<unsigned int>& i_indexBuffer, 
		unsigned int i_triangleIndex,
		BoundingBox& i_boundingBox)
	{
		float boxcenter[3];
		float boxhalfsize[3];
		float triverts[3][3];
		boxcenter[0] = i_boundingBox.centerX; boxcenter[1] = i_boundingBox.centerY; boxcenter[2] = i_boundingBox.centerZ;
		boxhalfsize[0] = i_boundingBox.halfWidth; boxhalfsize[1] = i_boundingBox.halfWidth; boxhalfsize[2] = i_boundingBox.halfWidth;
		triverts[0][0] = i_vertexBuffer[i_indexBuffer[i_triangleIndex*3]].x; triverts[0][1] = i_vertexBuffer[i_indexBuffer[i_triangleIndex*3]].y; triverts[0][2] = i_vertexBuffer[i_indexBuffer[i_triangleIndex*3]].z;
		triverts[1][0] = i_vertexBuffer[i_indexBuffer[i_triangleIndex*3+1]].x; triverts[1][1] = i_vertexBuffer[i_indexBuffer[i_triangleIndex*3+1]].y; triverts[1][2] = i_vertexBuffer[i_indexBuffer[i_triangleIndex*3+1]].z;
		triverts[2][0] = i_vertexBuffer[i_indexBuffer[i_triangleIndex * 3 + 2]].x; triverts[2][1] = i_vertexBuffer[i_indexBuffer[i_triangleIndex * 3 + 2]].y; triverts[2][2] = i_vertexBuffer[i_indexBuffer[i_triangleIndex * 3 + 2]].z;
		/*    use separating axis theorem to test overlap between triangle and box */

		/*    need to test for overlap in these directions: */

		/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */

		/*       we do not even need to test these) */

		/*    2) normal of the triangle */

		/*    3) crossproduct(edge from tri, {x,y,z}-directin) */

		/*       this gives 3x3=9 more tests */

		float v0[3], v1[3], v2[3];

		//   float axis[3];

		float min, max, p0, p1, p2, rad, fex, fey, fez;		// -NJMP- "d" local variable removed

		float normal[3], e0[3], e1[3], e2[3];



		/* This is the fastest branch on Sun */

		/* move everything so that the boxcenter is in (0,0,0) */

		v0[0] = triverts[0][0] - boxcenter[0];
		v0[1] = triverts[0][1] - boxcenter[1];
		v0[2] = triverts[0][2] - boxcenter[2];

		v1[0] = triverts[1][0] - boxcenter[0];
		v1[1] = triverts[1][1] - boxcenter[1];
		v1[2] = triverts[1][2] - boxcenter[2];

		v2[0] = triverts[2][0] - boxcenter[0];
		v2[1] = triverts[2][1] - boxcenter[1];
		v2[2] = triverts[2][2] - boxcenter[2];


		/* compute triangle edges */
		e0[0] = v1[0] - v0[0];/* tri edge 0 */
		e0[1] = v1[1] - v0[1];
		e0[2] = v1[2] - v0[2];

		e1[0] = v2[0] - v1[0];/* tri edge 1 */
		e1[1] = v2[1] - v1[1];
		e1[2] = v2[2] - v1[2];

		e2[0] = v0[0] - v2[0];/* tri edge 2 */
		e2[1] = v0[1] - v2[1];
		e2[2] = v0[2] - v2[2];


		/* Bullet 3:  */

		/*  test the 9 tests first (this was faster) */

		fex = fabsf(e0[0]);
		fey = fabsf(e0[1]);
		fez = fabsf(e0[2]);

		p0 = e0[2] * v0[1] - e0[1] * v0[2];
		p2 = e0[2] * v2[1] - e0[1] * v2[2];
		if (p0<p2) { min = p0; max = p2; }
		else { min = p2; max = p0; } 
		rad = fez * boxhalfsize[1] + fey * boxhalfsize[2];
		if (min>rad || max<-rad) return 0;

		p0 = -e0[2] * v0[0] + e0[0] * v0[2];
		p2 = -e0[2] * v2[0] + e0[0] * v2[2];
		if (p0<p2) { min = p0; max = p2; }
		else { min = p2; max = p0; } 
		rad = fez * boxhalfsize[0] + fex * boxhalfsize[2];
		if (min>rad || max<-rad) return 0;

		p1 = e0[1] * v1[0] - e0[0] * v1[1];
		p2 = e0[1] * v2[0] - e0[0] * v2[1];
		if (p2<p1) { min = p2; max = p1; }
		else { min = p1; max = p2; } 
		rad = fey * boxhalfsize[0] + fex * boxhalfsize[1];
		if (min>rad || max<-rad) return 0;


		fex = fabsf(e1[0]);
		fey = fabsf(e1[1]);
		fez = fabsf(e1[2]);


		p0 = e1[2] * v0[1] - e1[1] * v0[2];
		p2 = e1[2] * v2[1] - e1[1] * v2[2];
		if (p0<p2) { min = p0; max = p2; }
		else { min = p2; max = p0; }
		rad = fez * boxhalfsize[1] + fey * boxhalfsize[2];
		if (min>rad || max<-rad) return 0;

		p0 = -e1[2] * v0[0] + e1[0] * v0[2];
		p2 = -e1[2] * v2[0] + e1[0] * v2[2];
		if (p0<p2) { min = p0; max = p2; }
		else { min = p2; max = p0; } 
		rad = fez * boxhalfsize[0] + fex * boxhalfsize[2];
		if (min>rad || max<-rad) return 0;

		p0 = e1[1] * v0[0] - e1[0] * v0[1];
		p1 = e1[1] * v1[0] - e1[0] * v1[1];
		if (p0<p1) { min = p0; max = p1; }
		else { min = p1; max = p0; } 
		rad = fey * boxhalfsize[0] + fex * boxhalfsize[1];
		if (min>rad || max<-rad) return 0;


		fex = fabsf(e2[0]);
		fey = fabsf(e2[1]);
		fez = fabsf(e2[2]);

		p0 = e2[2] * v0[1] - e2[1] * v0[2];
		p1 = e2[2] * v1[1] - e2[1] * v1[2];
		if (p0<p1) { min = p0; max = p1; }
		else { min = p1; max = p0; } 
		rad = fez * boxhalfsize[1] + fey * boxhalfsize[2];
		if (min>rad || max<-rad) return 0;

		p0 = -e2[2] * v0[0] + e2[0] * v0[2];
		p1 = -e2[2] * v1[0] + e2[0] * v1[2];
		if (p0<p1) { min = p0; max = p1; }
		else { min = p1; max = p0; } 
		rad = fez * boxhalfsize[0] + fex * boxhalfsize[2];
		if (min>rad || max<-rad) return 0;

		p1 = e2[1] * v1[0] - e2[0] * v1[1];
		p2 = e2[1] * v2[0] - e2[0] * v2[1];
		if (p2<p1) { min = p2; max = p1; }
		else { min = p1; max = p2; } 
		rad = fey * boxhalfsize[0] + fex * boxhalfsize[1];
		if (min>rad || max<-rad) return 0;


		/* Bullet 1: */

		/*  first test overlap in the {x,y,z}-directions */

		/*  find min, max of the triangle each direction, and test for overlap in */

		/*  that direction -- this is equivalent to testing a minimal AABB around */

		/*  the triangle against the AABB */



		/* test in X-direction */
		min = max = v0[0];
		if (v1[0]<min) min = v1[0];
		if (v1[0]>max) max = v1[0];
		if (v2[0]<min) min = v2[0];
		if (v2[0]>max) max = v2[0];

		if (min>boxhalfsize[0] || max<-boxhalfsize[0]) return 0;

		/* test in Y-direction */
		min = max = v0[1];
		if (v1[1]<min) min = v1[1];
		if (v1[1]>max) max = v1[1];
		if (v2[1]<min) min = v2[1];
		if (v2[1]>max) max = v2[1];
		if (min>boxhalfsize[1] || max<-boxhalfsize[1]) return 0;

		/* test in Z-direction */
		min = max = v0[2];
		if (v1[2]<min) min = v1[2];
		if (v1[2]>max) max = v1[2];
		if (v2[2]<min) min = v2[2];
		if (v2[2]>max) max = v2[2];

		if (min>boxhalfsize[2] || max<-boxhalfsize[2]) return 0;



		/* Bullet 2: */

		/*  test if the box intersects the plane of the triangle */

		/*  compute plane equation of triangle: normal*x+d=0 */
		normal[0] = e0[1] * e1[2] - e0[2] * e1[1];
		normal[1] = e0[2] * e1[0] - e0[0] * e1[2];
		normal[2] = e0[0] * e1[1] - e0[1] * e1[0];
		// -NJMP- (line removed here)

		if (!planeBoxOverlap(normal, v0, boxhalfsize)) return 0;	// -NJMP-



		return 1;   /* box and triangle overlaps */

	}
}
