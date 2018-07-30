/* @@@LICENSE
*
*      Copyright (c) 2016-18 LG Electronics, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-License-Identifier: Apache-2.0
* LICENSE@@@ */


#include "errors.h"
#include "settings/settingsadapter.h"
#include "aspectratiosetting.h"
#include "video/videoservice.h"

AspectRatioSetting::AspectRatioSetting(LS::Handle& serviceHandle, VideoService &video)
		:mSettingsAdapter(serviceHandle,this, &AspectRatioSetting::processResponse)
		,mVideoService(video)
		,mCurrentAspectMode(MINUMUM)
		,mAllDirZoomVRatio(12),mAllDirZoomVPosition(0),mAllDirZoomHRatio(12),mAllDirZoomHPosition(0), mVertZoomVRatio(0),mVertZoomVPosition(0)
		,mJustScan(false)

{
	video.setAppIdChangedObserver(this,&AspectRatioSetting::fetchAspectRatioforApp);
}


static std::unordered_map<std::string, ARC_MODE_NAME_MAP_T> ARC_MODE_MAP =
		{{"16x9", MODE_16_9},
		 {"original", MODE_ORIGINAL},
		 {"4x3", MODE_4_3},
		 {"vertZoom", MODE_VERTICALZOOM},
		 {"allDirZoom", MODE_ALLDIRECTIONZOOM}
		};
static std::unordered_map<std::string, bool> SCAN_ON_OFF =
		{{"on", true},
		 {"off", false}
		};

void AspectRatioSetting::processResponse(pbnjson::JValue& settingsResponse)
{
	// vZoomPosition range is dependent on vZoomRatio. Recheck this
	// when vZoomRatio is -8(min), vZoomPosition has range (-1 to 1)
	// when vZoomRatio is +9(max), vZoomPosition can take (-18 to 18)

	LSHelpers::JsonParser parser = LSHelpers::JsonParser(settingsResponse).getObject("settings");
	parser.getAndMap("arcPerApp", mCurrentAspectMode, ARC_MODE_MAP).optional();
	parser.get("allDirZoomHPosition", mAllDirZoomHPosition).optional().min(-AllDirZoomRange).max( AllDirZoomRange);
	parser.get("allDirZoomHRatio", mAllDirZoomHRatio).optional().min(0).max( AllDirZoomRange);
	parser.get("allDirZoomVPosition", mAllDirZoomVPosition).optional().min(-AllDirZoomRange).max( AllDirZoomRange);
	parser.get("allDirZoomVRatio", mAllDirZoomVRatio).optional().min(0).max( AllDirZoomRange);
	parser.get("vertZoomVRatio", mVertZoomVRatio).optional().min(-VertZoomRange+1).max(VertZoomRange);
	auto posRange = VertZoomRange  + mVertZoomVRatio;
	parser.get("vertZoomVPosition", mVertZoomVPosition).optional().min(-posRange).max( posRange );
	parser.getAndMap("justScan", mJustScan, SCAN_ON_OFF).optional();

	FINISH_PARSE_OR_RETURN_FALSE(parser);

    LOG_DEBUG("Aspect Ratio configured as : mCurrentAspectMode: %d alldirZooms: %d %d %d %d VertZooms:%d %d ",
          mCurrentAspectMode, mAllDirZoomHPosition,mAllDirZoomHRatio, mAllDirZoomVPosition,mAllDirZoomVRatio,
          mVertZoomVRatio, mVertZoomVPosition);
	mVideoService.setAspectRatio(mCurrentAspectMode, mAllDirZoomHPosition, mAllDirZoomHRatio, mAllDirZoomVPosition,
	                             mAllDirZoomVRatio, mVertZoomVRatio, mVertZoomVPosition);
	return ;
}

void AspectRatioSetting::fetchAspectRatioforApp(std::string& appId)
{
	//cancel ARC subscription for previous appId and subscribe with new appId.
	mSettingsAdapter.subscribeToSettings(pbnjson::JObject{{"subscribe", true},
	                                              {"category",  "aspectRatio"},
			                                      {"dimension",{{"input", "default"},
					                              {"resolution", "x"}, {"twinMode", "off"}}},
	                                              {"app_id",    appId}});
}