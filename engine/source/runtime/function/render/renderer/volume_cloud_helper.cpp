#include "volume_cloud_helper.h"

namespace MoYu
{
	namespace VolumeCloudSpace
	{

		const float mWindSpeedMultiplier = 175.0f;

		CloudsConsCB InitDefaultCloudsCons(float time, glm::float3 wind)
		{
			CloudsConstants cloudsCons{};

			cloudsCons.AmbientColor = glm::float4(102.0f / 255.0f, 104.0f / 255.0f, 105.0f / 255.0f, 1.0f);
			cloudsCons.WindDir = glm::float4(glm::normalize(wind), 1);
			cloudsCons.WindSpeed = mWindSpeedMultiplier;
			cloudsCons.Time = time;
			cloudsCons.Crispiness = 43.0f;
			cloudsCons.Curliness = 1.1f;
			cloudsCons.Coverage = 0.305f;
			cloudsCons.Absorption = 0.0015f;
			cloudsCons.CloudsBottomHeight = 2340.0f;
			cloudsCons.CloudsTopHeight = 16400.0f;
			cloudsCons.DensityFactor = 0.006f;
			cloudsCons.DistanceToFadeFrom = 58000.0f;
			cloudsCons.DistanceOfFade = 10000.0f;
			cloudsCons.PlanetRadius = 636000.0f;
			cloudsCons.UpsampleRatio = glm::float2(1, 1);

			return CloudsConsCB {cloudsCons};
		}

	}

}
