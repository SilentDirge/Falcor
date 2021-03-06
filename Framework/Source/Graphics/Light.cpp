/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "Framework.h"
#include "light.h"
#include "Utils/Gui.h"
#include "API/ConstantBuffer.h"
#include "API/Buffer.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "Data/VertexAttrib.h"
#include "Graphics/Model/Model.h"


namespace Falcor
{
    bool checkOffset(size_t cbOffset, size_t cppOffset, const char* field)
    {
        if (cbOffset != cppOffset)
        {
            logError("Light::LightData::" + std::string(field) + " CB offset mismatch. CB offset is " + std::to_string(cbOffset) + ", C++ data offset is " + std::to_string(cppOffset));
            return false;
        }
        return true;
    }

#if _LOG_ENABLED
#define check_offset(_a) {static bool b = true; if(b) {assert(checkOffset(pBuffer->getVariableOffset(varName + "." + #_a) - offset, offsetof(LightData, _a), #_a));} b = false;}
#else
#define check_offset(_a)
#endif

    uint32_t Light::sCount = 0;

    Light::Light()
    {
        mIndex = sCount;
        sCount++;
    }

    void Light::setIntoConstantBuffer(ConstantBuffer* pBuffer, const std::string& varName)
    {
        static const size_t dataSize = sizeof(LightData) - sizeof(MaterialData);
        static_assert(dataSize % sizeof(float) * 4 == 0, "LightData size should be a multiple of 16");
        static_assert(sizeof(LightData) - sizeof(MaterialData) == offsetof(LightData, material), "'material' must be the last field in LightData");

        size_t offset = pBuffer->getVariableOffset(varName + ".worldPos");
        if (offset == ConstantBuffer::kInvalidOffset)
        {
            logWarning("AreaLight::setIntoConstantBuffer() - variable \"" + varName + "\"not found in constant buffer\n");
            return;
        }

        check_offset(worldDir);
        check_offset(intensity);
        check_offset(aabbMin);
        check_offset(aabbMax);
        check_offset(transMat);
        check_offset(numIndices);
        assert(offset + dataSize <= pBuffer->getSize());

        // Set everything except for the material
        pBuffer->setBlob(&mData, offset, dataSize);
        if (mData.type == LightArea)
        {
            assert(0);
        }
    }

    void Light::resetGlobalIdCounter()
    {
        sCount = 0;
    }

    glm::vec3 Light::getColorForUI()
    {
        if ((mUiLightIntensityColor * mUiLightIntensityScale) != mData.intensity)
        {
            float mag = max(mData.intensity.x, max(mData.intensity.y, mData.intensity.z));
            if (mag <= 1.f)
            {
                mUiLightIntensityColor = mData.intensity;
                mUiLightIntensityScale = 1.0f;
            }
            else
            {
                mUiLightIntensityColor = mData.intensity / mag;
                mUiLightIntensityScale = mag;
            }
        }

        return mUiLightIntensityColor;
    }

    void updateAreaLightIntensity(LightData& light)
    {
        // Update material
        if (light.type == LightArea)
        {
            for (int i = 0; i < MatMaxLayers; ++i)
            {
                if (light.material.desc.layers[i].type == MatEmissive)
                {
                    light.material.values.layers[i].albedo = v4(light.intensity, 0.f);
                }
            }
        }
    }

    void Light::setColorFromUI(const glm::vec3& uiColor)
    {
        mUiLightIntensityColor = uiColor;
        mData.intensity = (mUiLightIntensityColor * mUiLightIntensityScale);
        updateAreaLightIntensity(mData);
    }

    float Light::getIntensityForUI()
    {
        if ((mUiLightIntensityColor * mUiLightIntensityScale) != mData.intensity)
        {
            float mag = max(mData.intensity.x, max(mData.intensity.y, mData.intensity.z));
            if (mag <= 1.f)
            {
                mUiLightIntensityColor = mData.intensity;
                mUiLightIntensityScale = 1.0f;
            }
            else
            {
                mUiLightIntensityColor = mData.intensity / mag;
                mUiLightIntensityScale = mag;
            }
        }

        return mUiLightIntensityScale;
    }

    void Light::setIntensityFromUI(float intensity)
    {
        mUiLightIntensityScale = intensity;
        mData.intensity = (mUiLightIntensityColor * mUiLightIntensityScale);
        updateAreaLightIntensity(mData);
    }

    void Light::renderUI(Gui* pGui)
    {
        glm::vec3 color = getColorForUI();
        if (pGui->addRgbColor("Color", color))
        {
        }
        float intensity = getIntensityForUI();
        if (pGui->addFloatVar("Intensity", intensity))
        {
            setIntensityFromUI(intensity);
        }
    }

	DirectionalLight::DirectionalLight() : mDistance(-1.0f)
    {
        mData.type = LightDirectional;
        mName = "dirLight" + std::to_string(mIndex);
    }

    DirectionalLight::SharedPtr DirectionalLight::create()
    {
        DirectionalLight* pLight = new DirectionalLight();
        return SharedPtr(pLight);
    }

    DirectionalLight::~DirectionalLight() = default;

    void DirectionalLight::renderUI(Gui* pGui)
    {
        if (pGui->addDirectionWidget("Direction", mData.worldDir))
        {
            setWorldDirection(mData.worldDir);
        }
        Light::renderUI(pGui);
    }

    void DirectionalLight::setWorldDirection(const glm::vec3& dir)
    {
        mData.worldDir = normalize(dir);
        mData.worldPos = mCenter - mData.worldDir * mDistance; // Move light's position sufficiently far away
    }

    void DirectionalLight::setWorldParams(const glm::vec3& center, float radius)
    {
        mDistance = radius;
        mCenter = center;
        mData.worldPos = mCenter - mData.worldDir * mDistance; // Move light's position sufficiently far away
    }

    void DirectionalLight::prepareGPUData()
    {
    }

    void DirectionalLight::unloadGPUData()
    {
    }

    void DirectionalLight::move(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
    {
        logError("DirectionalLight::move() is not used and thus not implemented for now.");
    }

    PointLight::SharedPtr PointLight::create()
    {
        PointLight* pLight = new PointLight;
        return SharedPtr(pLight);
    }

    PointLight::PointLight()
    {
        mData.type = LightPoint;
        mName = "pointLight" + std::to_string(mIndex);
    }

    PointLight::~PointLight() = default;

    void PointLight::renderUI(Gui* pGui)
    {
        pGui->addFloat3Var("World Position", mData.worldPos, -FLT_MAX, FLT_MAX);
        pGui->addDirectionWidget("Direction", mData.worldDir);

        if (pGui->addFloatVar("Opening Angle", mData.openingAngle, 0.f, (float)M_PI))
        {
            setOpeningAngle(mData.openingAngle);
        }
        if (pGui->addFloatVar("Penumbra Width", mData.penumbraAngle, 0.f, (float)M_PI))
        {
            setPenumbraAngle(mData.penumbraAngle);
        }
        Light::renderUI(pGui);
    }

    void PointLight::setOpeningAngle(float openingAngle)
    {
        openingAngle = glm::clamp(openingAngle, 0.f, (float)M_PI);
        mData.openingAngle = openingAngle;
        /* Prepare an auxiliary cosine of the opening angle to quickly check whether we're within the cone of a spot light */
        mData.cosOpeningAngle = cos(openingAngle);
    }

    void PointLight::prepareGPUData()
    {
    }

    void PointLight::unloadGPUData()
    {
    }

    void PointLight::move(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
    {
        mData.worldPos = position;
        mData.worldDir = target - position;
    }

    AreaLight::SharedPtr AreaLight::create()
    {
        AreaLight* pLight = new AreaLight;
        return SharedPtr(pLight);
    }

    AreaLight::AreaLight()
    {
        mData.type = LightArea;
        mName = std::string("areaLight") + std::to_string(mIndex);
    }

    AreaLight::~AreaLight() = default;

    void AreaLight::renderUI(Gui* pGui)
    {
		logError("AreaLight::renderUI() is not used and thus not implemented for now.");
        return;
        // DISABLED_FOR_D3D12
#if 0        
        std::string posGroup = "worldPos" + std::to_string(mIndex);
        
        if(mMeshData.pMesh)
        {
            mat4& mx = (mat4&)mMeshData.pMesh->getInstanceMatrices()[mMeshData.instanceId];
            pGui->addFloatVar("x", &mx[3].x, posGroup, -FLT_MAX, FLT_MAX);
            pGui->addFloatVar("y", &mx[3].y, posGroup, -FLT_MAX, FLT_MAX);
            pGui->addFloatVar("z", &mx[3].z, posGroup, -FLT_MAX, FLT_MAX);
            pGui->nestGroups(uiGroup, posGroup);
            pGui->setVarTitle(posGroup, "World Position");
        }

        //pGui->addDir3FVar("Direction", &mData.worldDir, uiGroup);
        pGui->addRgbColorWithCallback("Color" + std::to_string(mIndex), SetColorCB, GetColorCB, this, uiGroup);
        pGui->addFloatVarWithCallback("Intensity" + std::to_string(mIndex), SetIntensityCB, GetIntensityCB, this, uiGroup, 0.0f, 1000000.0f, 0.1f);
#endif
    }

    void AreaLight::setIntoConstantBuffer(ConstantBuffer* pBuffer, const std::string& varName)
    {
        // Upload data to GPU
        prepareGPUData();

        // Call base class method;
        Light::setIntoConstantBuffer(pBuffer, varName);
    }

    void AreaLight::prepareGPUData()
    {
        // DISABLED_FOR_D3D12
        // Set OGL buffer pointers for indices, vertices, and texcoord
// 		if (mData.indexPtr.ptr == 0ull)
// 		{
// 			mData.indexPtr.ptr = mIndexBuf->makeResident();
// 			mData.vertexPtr.ptr = mVertexBuf->makeResident();
// 			if (mTexCoordBuf)
// 				mData.texCoordPtr.ptr = mTexCoordBuf->makeResident();
// 			// Store the mesh CDF buffer id
// 			mData.meshCDFPtr.ptr = mMeshCDFBuf->makeResident();
// 		}
// 		mData.numIndices = uint32_t(mIndexBuf->getSize() / sizeof(glm::ivec3));
// 
// 		// Get the surface area of the geometry mesh
// 		mData.surfaceArea = mSurfaceArea;
// 
//		mData.tangent = mTangent;
//		mData.bitangent = mBitangent;
//
// 		// Fetch the mesh instance transformation
// 		mData.transMat = mMeshData.pMesh->getInstanceMatrix(mMeshData.instanceId);
// 
// 		// Copy the material data
// 		const Material::SharedPtr& pMaterial = mMeshData.pMesh->getMaterial();
// 		if (pMaterial)
// 			memcpy(&mData.material, &pMaterial->getData(), sizeof(MaterialData));
    }

    void AreaLight::unloadGPUData()
    {
        // Unload GPU data by calling evict()
        mIndexBuf->evict();
        mVertexBuf->evict();
        if (mTexCoordBuf)
            mTexCoordBuf->evict();
        mMeshCDFBuf->evict();
    }

    void AreaLight::setMeshData(const Model::MeshInstance::SharedPtr& pMeshInstance)
{
		if (pMeshInstance && pMeshInstance != mpMeshInstance)
        {
            const auto& pMesh = pMeshInstance->getObject();
            assert(pMesh != nullptr);

            mpMeshInstance = pMeshInstance;

            const auto& vao = pMesh->getVao();

            setIndexBuffer(vao->getIndexBuffer());

            int32_t posIdx = vao->getElementIndexByLocation(VERTEX_POSITION_LOC).vbIndex;
            assert(posIdx != Vao::ElementDesc::kInvalidIndex);
            setPositionsBuffer(vao->getVertexBuffer(posIdx));

            const int32_t uvIdx = vao->getElementIndexByLocation(VERTEX_TEXCOORD_LOC).vbIndex;
            bool hasUv = uvIdx != Vao::ElementDesc::kInvalidIndex;
            if (hasUv)
            {
                setTexCoordBuffer(vao->getVertexBuffer(VERTEX_TEXCOORD_LOC));
            }

            // Compute surface area of the mesh and generate probability
            // densities for importance sampling a triangle mesh
            computeSurfaceArea();

            // Check if this mesh has a material
            const Material::SharedPtr& pMaterial = pMesh->getMaterial();
            if (pMaterial)
            {
                for (uint32_t layerId = 0; layerId < pMaterial->getNumLayers(); ++layerId)
                {
                    const Material::Layer l = pMaterial->getLayer(layerId);
                    if (l.type == Material::Layer::Type::Emissive)
                    {
                        mData.intensity = vec3(l.albedo);
                        break;
                    }
                }
            }
        }
    }

    void AreaLight::computeSurfaceArea()
    {
        if (mpMeshInstance && mVertexBuf && mIndexBuf)
        {
            const auto& pMesh = mpMeshInstance->getObject();
            assert(pMesh != nullptr);

            // Read data from the buffers
            std::vector<glm::ivec3> indices(pMesh->getPrimitiveCount());
            mIndexBuf->readData(indices.data(), 0, mIndexBuf->getSize());
            std::vector<vec3> vertices(mVertexBuf->getSize() / sizeof(vec3));
            mVertexBuf->readData(vertices.data(), 0, mVertexBuf->getSize());

			if (mpMeshInstance->getObject()->getPrimitiveCount() != 2 || mpMeshInstance->getObject()->getVertexCount() != 4)
            {
                logWarning("Only support sampling of rectangular light sources made of 2 triangles.");
                return;
            }

            // Calculate surface area of the mesh
            mSurfaceArea = 0.f;
            mMeshCDF.push_back(0.f);
            for (uint32_t i = 0; i < pMesh->getPrimitiveCount(); ++i)
            {
                glm::ivec3 pId = indices[i];
                const vec3 p0(vertices[pId.x]), p1(vertices[pId.y]), p2(vertices[pId.z]);

                mSurfaceArea += 0.5f * glm::length(glm::cross(p1 - p0, p2 - p0));

                // Add an entry using surface area measure as the discrete probability
                mMeshCDF.push_back(mMeshCDF[mMeshCDF.size() - 1] + mSurfaceArea);
            }

            // Normalize the probability densities
            if (mSurfaceArea > 0.f)
            {
                float invSurfaceArea = 1.f / mSurfaceArea;
                for (uint32_t i = 1; i < mMeshCDF.size(); ++i)
                {
                    mMeshCDF[i] *= invSurfaceArea;
                }

                mMeshCDF[mMeshCDF.size() - 1] = 1.f;
            }

			// Calculate basis tangent vectors and their lengths
			ivec3 pId = indices[0];
			const vec3 p0(vertices[pId.x]), p1(vertices[pId.y]), p2(vertices[pId.z]);

			mTangent = p0 - p1;
			mBitangent = p2 - p1;

            // Create a CDF buffer
            mMeshCDFBuf.reset();
            mMeshCDFBuf = Buffer::create(sizeof(mMeshCDF[0])*mMeshCDF.size(), Buffer::BindFlags::Vertex, Buffer::CpuAccess::None, mMeshCDF.data());

            // Set the world position and world direction of this light
            if (!vertices.empty() && !indices.empty())
            {
                glm::vec3 boxMin = vertices[0];
                glm::vec3 boxMax = vertices[0];
                for (uint32_t id = 1; id < vertices.size(); ++id)
                {
                    boxMin = glm::min(boxMin, vertices[id]);
                    boxMax = glm::max(boxMax, vertices[id]);
                }

                mData.worldPos = BoundingBox::fromMinMax(boxMin, boxMax).center;

                // This holds only for planar light sources
                const glm::vec3& p0 = vertices[indices[0].x];
                const glm::vec3& p1 = vertices[indices[0].y];
                const glm::vec3& p2 = vertices[indices[0].z];

                // Take the normal of the first triangle as a light normal
                mData.worldDir = normalize(cross(p1 - p0, p2 - p0));

                // Save the axis-aligned bounding box
                mData.aabbMin = boxMin;
                mData.aabbMax = boxMax;
            }
        }
    }

    Light::SharedPtr AreaLight::createAreaLight(const Model::MeshInstance::SharedPtr& pMeshInstance)
    {
        // Create an area light
        AreaLight::SharedPtr pAreaLight = AreaLight::create();
        if (pAreaLight)
        {
            // Set the geometry mesh
            pAreaLight->setMeshData(pMeshInstance);
        }

        return pAreaLight;
    }

    void AreaLight::createAreaLightsForModel(const Model::SharedPtr& pModel, std::vector<Light::SharedPtr>& areaLights)
    {
        assert(pModel);

        // Get meshes for this model
        for (uint32_t meshId = 0; meshId < pModel->getMeshCount(); ++meshId)
        {
            const Mesh::SharedPtr& pMesh = pModel->getMesh(meshId);

            // Obtain mesh instances for this mesh
            for (uint32_t instanceId = 0; instanceId < pModel->getMeshInstanceCount(meshId); ++instanceId)
            {
                // Check if this mesh has a material
                const Material::SharedPtr& pMaterial = pMesh->getMaterial();
                if (pMaterial)
                {
                    // Check for emissive layers
                    const uint32_t numLayers = pMaterial->getNumLayers();
                    for (uint32_t layerId = 0; layerId < numLayers; ++layerId)
                    {
                        const Material::Layer l = pMaterial->getLayer(layerId);
                        if (l.type == Material::Layer::Type::Emissive)
                        {
                            // Create an area light for an emissive material
                            areaLights.push_back(createAreaLight(pModel->getMeshInstance(meshId, instanceId)));
                            break;
                        }
                    }
                }
            }
        }
    }

    void AreaLight::move(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up)
    {
        mpMeshInstance->move(position, target, up);
    }
}
