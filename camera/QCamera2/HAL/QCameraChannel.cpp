/* Copyright (c) 2012-2015, The Linux Foundataion. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#define LOG_TAG "QCameraChannel"

#include <utils/Errors.h>
#include "QCameraParameters.h"
#include "QCamera2HWI.h"
#include "QCameraChannel.h"

using namespace android;

namespace qcamera {

/*===========================================================================
 * FUNCTION   : QCameraChannel
 *
 * DESCRIPTION: constrcutor of QCameraChannel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *
 * RETURN     : none
 *==========================================================================*/
QCameraChannel::QCameraChannel(uint32_t cam_handle,
                               mm_camera_ops_t *cam_ops)
{
    m_camHandle = cam_handle;
    m_camOps = cam_ops;
    m_bIsActive = false;
    m_bAllowDynBufAlloc = false;

    m_handle = 0;
}

/*===========================================================================
 * FUNCTION   : QCameraChannel
 *
 * DESCRIPTION: default constrcutor of QCameraChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraChannel::QCameraChannel()
{
    m_camHandle = 0;
    m_camOps = NULL;
    m_bIsActive = false;

    m_handle = 0;
}

/*===========================================================================
 * FUNCTION   : ~QCameraChannel
 *
 * DESCRIPTION: destructor of QCameraChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraChannel::~QCameraChannel()
{
    if (m_bIsActive) {
        stop();
    }
    for (size_t i = 0; i < mStreams.size(); i++) {
        if (mStreams[i] != NULL) {
                if (m_handle == mStreams[i]->getChannelHandle()) {
                    delete mStreams[i];
                }
        }
    }
    mStreams.clear();
    m_camOps->delete_channel(m_camHandle, m_handle);
    m_handle = 0;
}

/*===========================================================================
 * FUNCTION   : deleteChannel
 *
 * DESCRIPTION: deletes a camera channel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
void QCameraChannel::deleteChannel()
{
    if (m_bIsActive) {
        stop();
    }
    for (size_t i = 0; i < mStreams.size(); i++) {
        if ((mStreams[i] != NULL) && (m_handle == mStreams[i]->getChannelHandle())) {
            mStreams[i]->deleteStream();
        }
    }
    m_camOps->delete_channel(m_camHandle, m_handle);
}

/*===========================================================================
 * FUNCTION   : setStreamSyncCB
 *
 * DESCRIPTION: reg callback function to stream of stream type
 *
 * PARAMETERS :
 *    @stream_type : Stream type for which callback needs to be registered.
 *    @stream_cb   : Callback function

 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              non-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::setStreamSyncCB (cam_stream_type_t stream_type,
        stream_cb_routine stream_cb)
{
    int32_t rc = UNKNOWN_ERROR;
    for (size_t i = 0; i < mStreams.size(); i++) {
        if ((mStreams[i] != NULL) && (stream_type == mStreams[i]->getMyType())) {
            rc = mStreams[i]->setSyncDataCB(stream_cb);
            break;
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : init
 *
 * DESCRIPTION: initialization of channel
 *
 * PARAMETERS :
 *   @attr    : channel bundle attribute setting
 *   @dataCB  : data notify callback
 *   @userData: user data ptr
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::init(mm_camera_channel_attr_t *attr,
                             mm_camera_buf_notify_t dataCB,
                             void *userData)
{
    m_handle = m_camOps->add_channel(m_camHandle,
                                      attr,
                                      dataCB,
                                      userData);
    if (m_handle == 0) {
        ALOGE("%s: Add channel failed", __func__);
        return UNKNOWN_ERROR;
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : addStream
 *
 * DESCRIPTION: add a stream into channel
 *
 * PARAMETERS :
 *   @allocator      : stream related buffer allocator
 *   @streamInfoBuf  : ptr to buf that contains stream info
 *   @miscBuf        : ptr to buf that contains misc buffers
 *   @minStreamBufNum: number of stream buffers needed
 *   @paddingInfo    : padding information
 *   @stream_cb      : stream data notify callback
 *   @userdata       : user data ptr
 *   @bDynAllocBuf   : flag indicating if allow allocate buffers in 2 steps
 *   @online_rotation: rotation applied online
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::addStream(QCameraAllocator &allocator,
        QCameraHeapMemory *streamInfoBuf, QCameraHeapMemory *miscBuf,
        uint8_t minStreamBufNum, cam_padding_info_t *paddingInfo,
        stream_cb_routine stream_cb, void *userdata, bool bDynAllocBuf,
        bool bDeffAlloc, cam_rotation_t online_rotation)
{
    int32_t rc = NO_ERROR;
    if (mStreams.size() >= MAX_STREAM_NUM_IN_BUNDLE) {
        ALOGE("%s: stream number (%zu) exceeds max limit (%d)",
              __func__, mStreams.size(), MAX_STREAM_NUM_IN_BUNDLE);
        if (streamInfoBuf != NULL) {
            streamInfoBuf->deallocate();
            delete streamInfoBuf;
            streamInfoBuf = NULL;
        }
        return BAD_VALUE;
    }
    QCameraStream *pStream = new QCameraStream(allocator,
            m_camHandle, m_handle, m_camOps, paddingInfo, bDeffAlloc,
            online_rotation);
    if (pStream == NULL) {
        ALOGE("%s: No mem for Stream", __func__);
        if (streamInfoBuf != NULL) {
            streamInfoBuf->deallocate();
            delete streamInfoBuf;
            streamInfoBuf = NULL;
        }
        return NO_MEMORY;
    }

    rc = pStream->init(streamInfoBuf, miscBuf, minStreamBufNum,
                       stream_cb, userdata, bDynAllocBuf);
    if (rc == 0) {
        mStreams.add(pStream);
    } else {
        delete pStream;
    }
    return rc;
}
/*===========================================================================
 * FUNCTION   : config
 *
 * DESCRIPTION: Configure any deffered channel streams
 *
 * PARAMETERS : None
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::config()
{
    int32_t rc = NO_ERROR;
    for (size_t i = 0; i < mStreams.size(); ++i) {
        if ( mStreams[i]->isDeffered() ) {
            rc = mStreams[i]->configStream();
            if (rc != NO_ERROR) {
                break;
            }
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : linkStream
 *
 * DESCRIPTION: link a stream into channel
 *
 * PARAMETERS :
 *   @ch      : Channel which the stream belongs to
 *   @stream  : Stream which needs to be linked
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::linkStream(QCameraChannel *ch, QCameraStream *stream)
{
    int32_t rc = NO_ERROR;

    if ((0 == m_handle) || (NULL == ch) || (NULL == stream)) {
        return NO_INIT;
    }

    int32_t handle = m_camOps->link_stream(m_camHandle,
            ch->getMyHandle(),
            stream->getMyHandle(),
            m_handle);
    if (0 == handle) {
        ALOGE("%s : Linking of stream failed", __func__);
        rc = INVALID_OPERATION;
    } else {
        mStreams.add(stream);
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : start
 *
 * DESCRIPTION: start channel, which will start all streams belong to this channel
 *
 * PARAMETERS : None
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::start()
{
    int32_t rc = NO_ERROR;

    if (mStreams.size() > 1) {
        // there is more than one stream in the channel
        // we need to notify mctl that all streams in this channel need to be bundled
        cam_bundle_config_t bundleInfo;
        memset(&bundleInfo, 0, sizeof(bundleInfo));
        rc = m_camOps->get_bundle_info(m_camHandle, m_handle, &bundleInfo);
        if (rc != NO_ERROR) {
            ALOGE("%s: get_bundle_info failed", __func__);
            return rc;
        }
        if (bundleInfo.num_of_streams > 1) {
            for (int i = 0; i < bundleInfo.num_of_streams; i++) {
                QCameraStream *pStream = getStreamByServerID(bundleInfo.stream_ids[i]);
                if (pStream != NULL) {
                    if (pStream->isTypeOf(CAM_STREAM_TYPE_METADATA)) {
                        // Skip metadata for reprocess now because PP module cannot handle meta data
                        // May need furthur discussion if Imaginglib need meta data
                        continue;
                    }

                    cam_stream_parm_buffer_t param;
                    memset(&param, 0, sizeof(cam_stream_parm_buffer_t));
                    param.type = CAM_STREAM_PARAM_TYPE_SET_BUNDLE_INFO;
                    param.bundleInfo = bundleInfo;
                    rc = pStream->setParameter(param);
                    if (rc != NO_ERROR) {
                        ALOGE("%s: stream setParameter for set bundle failed", __func__);
                        return rc;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < mStreams.size(); i++) {
        if ((mStreams[i] != NULL) &&
                (m_handle == mStreams[i]->getChannelHandle())) {
            mStreams[i]->start();
        }
    }
    rc = m_camOps->start_channel(m_camHandle, m_handle);

    if (rc != NO_ERROR) {
        for (size_t i = 0; i < mStreams.size(); i++) {
            if ((mStreams[i] != NULL) &&
                    (m_handle == mStreams[i]->getChannelHandle())) {
                mStreams[i]->stop();
            }
        }
    } else {
        m_bIsActive = true;
        for (size_t i = 0; i < mStreams.size(); i++) {
            if (mStreams[i] != NULL) {
                mStreams[i]->cond_signal();
            }
        }
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : stop
 *
 * DESCRIPTION: stop a channel, which will stop all streams belong to this channel
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::stop()
{
    int32_t rc = NO_ERROR;
    ssize_t linkedIdx = -1;

    if (!m_bIsActive) {
        return NO_INIT;
    }

    for (size_t i = 0; i < mStreams.size(); i++) {
        if (mStreams[i] != NULL) {
               if (m_handle == mStreams[i]->getChannelHandle()) {
                   mStreams[i]->stop();
               } else {
                   // Remove linked stream from stream list
                   linkedIdx = (ssize_t)i;
               }
        }
    }
    if (linkedIdx > 0) {
        mStreams.removeAt((size_t)linkedIdx);
    }

    rc = m_camOps->stop_channel(m_camHandle, m_handle);

    m_bIsActive = false;
    return rc;
}

/*===========================================================================
 * FUNCTION   : bufDone
 *
 * DESCRIPTION: return a stream buf back to kernel
 *
 * PARAMETERS :
 *   @recvd_frame  : stream buf frame to be returned
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::bufDone(mm_camera_super_buf_t *recvd_frame)
{
    int32_t rc = NO_ERROR;
    for (uint32_t i = 0; i < recvd_frame->num_bufs; i++) {
        if (recvd_frame->bufs[i] != NULL) {
            for (size_t j = 0; j < mStreams.size(); j++) {
                if (mStreams[j] != NULL &&
                        mStreams[j]->getMyHandle() == recvd_frame->bufs[i]->stream_id) {
                    rc = mStreams[j]->bufDone(recvd_frame->bufs[i]->buf_idx);
                    break; // break loop j
                }
            }
        }
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : processZoomDone
 *
 * DESCRIPTION: process zoom done event
 *
 * PARAMETERS :
 *   @previewWindoe : ptr to preview window ops table, needed to set preview
 *                    crop information
 *   @crop_info     : crop info as a result of zoom operation
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::processZoomDone(preview_stream_ops_t *previewWindow,
                                        cam_crop_data_t &crop_info)
{
    int32_t rc = NO_ERROR;
    for (size_t i = 0; i < mStreams.size(); i++) {
        if ((mStreams[i] != NULL) &&
                (m_handle == mStreams[i]->getChannelHandle())) {
            rc = mStreams[i]->processZoomDone(previewWindow, crop_info);
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : getStreamByHandle
 *
 * DESCRIPTION: return stream object by stream handle
 *
 * PARAMETERS :
 *   @streamHandle : stream handle
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
QCameraStream *QCameraChannel::getStreamByHandle(uint32_t streamHandle)
{
    for (size_t i = 0; i < mStreams.size(); i++) {
        if (mStreams[i] != NULL && mStreams[i]->getMyHandle() == streamHandle) {
            return mStreams[i];
        }
    }
    return NULL;
}

/*===========================================================================
 * FUNCTION   : getStreamByServerID
 *
 * DESCRIPTION: return stream object by stream server ID from daemon
 *
 * PARAMETERS :
 *   @serverID : stream server ID
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
QCameraStream *QCameraChannel::getStreamByServerID(uint32_t serverID)
{
    for (size_t i = 0; i < mStreams.size(); i++) {
        if (mStreams[i] != NULL && mStreams[i]->getMyServerID() == serverID) {
            return mStreams[i];
        }
    }
    return NULL;
}

/*===========================================================================
 * FUNCTION   : getStreamByIndex
 *
 * DESCRIPTION: return stream object by index of streams in the channel
 *
 * PARAMETERS :
 *   @index : index of stream in the channel
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
QCameraStream *QCameraChannel::getStreamByIndex(uint32_t index)
{
    if (index >= MAX_STREAM_NUM_IN_BUNDLE) {
        return NULL;
    }

    if (index < mStreams.size()) {
        return mStreams[index];
    }
    return NULL;
}

/*===========================================================================
 * FUNCTION   : UpdateStreamBasedParameters
 *
 * DESCRIPTION: update any stream based settings from parameters
 *
 * PARAMETERS :
 *   @param   : reference to parameters object
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraChannel::UpdateStreamBasedParameters(QCameraParameters &param)
{
    int32_t rc = NO_ERROR;
    if (param.isPreviewFlipChanged()) {
        // try to find preview stream
        for (size_t i = 0; i < mStreams.size(); i++) {
            if ((mStreams[i] != NULL) &&
                    (m_handle == mStreams[i]->getChannelHandle()) &&
                    (mStreams[i]->isTypeOf(CAM_STREAM_TYPE_PREVIEW) ||
                    (mStreams[i]->isOrignalTypeOf(CAM_STREAM_TYPE_PREVIEW))) ) {
                cam_stream_parm_buffer_t param_buf;
                memset(&param_buf, 0, sizeof(cam_stream_parm_buffer_t));
                param_buf.type = CAM_STREAM_PARAM_TYPE_SET_FLIP;
                param_buf.flipInfo.flip_mask =
                        (uint32_t)param.getFlipMode(CAM_STREAM_TYPE_PREVIEW);
                rc = mStreams[i]->setParameter(param_buf);
                if (rc != NO_ERROR) {
                    ALOGE("%s: set preview stream flip failed", __func__);
                }
            }
        }
    }
    if (param.isVideoFlipChanged()) {
        // try to find video stream
        for (size_t i = 0; i < mStreams.size(); i++) {
            if ((mStreams[i] != NULL) &&
                    (m_handle == mStreams[i]->getChannelHandle()) &&
                    (mStreams[i]->isTypeOf(CAM_STREAM_TYPE_VIDEO) ||
                    (mStreams[i]->isOrignalTypeOf(CAM_STREAM_TYPE_VIDEO))) ) {
                cam_stream_parm_buffer_t param_buf;
                memset(&param_buf, 0, sizeof(cam_stream_parm_buffer_t));
                param_buf.type = CAM_STREAM_PARAM_TYPE_SET_FLIP;
                param_buf.flipInfo.flip_mask =
                        (uint32_t)param.getFlipMode(CAM_STREAM_TYPE_VIDEO);
                rc = mStreams[i]->setParameter(param_buf);
                if (rc != NO_ERROR) {
                    ALOGE("%s: set video stream flip failed", __func__);
                }
            }
        }
    }
    if (param.isSnapshotFlipChanged()) {
        // try to find snapshot/postview stream
        for (size_t i = 0; i < mStreams.size(); i++) {
            if (mStreams[i] != NULL &&
                    (m_handle == mStreams[i]->getChannelHandle()) &&
                    (mStreams[i]->isTypeOf(CAM_STREAM_TYPE_SNAPSHOT) ||
                     mStreams[i]->isOrignalTypeOf(CAM_STREAM_TYPE_SNAPSHOT) ||
                     mStreams[i]->isTypeOf(CAM_STREAM_TYPE_POSTVIEW) ||
                 mStreams[i]->isOrignalTypeOf(CAM_STREAM_TYPE_POSTVIEW) ) ) {
                cam_stream_parm_buffer_t param_buf;
                memset(&param_buf, 0, sizeof(cam_stream_parm_buffer_t));
                param_buf.type = CAM_STREAM_PARAM_TYPE_SET_FLIP;
                param_buf.flipInfo.flip_mask =
                        (uint32_t)param.getFlipMode(CAM_STREAM_TYPE_SNAPSHOT);
                rc = mStreams[i]->setParameter(param_buf);
                if (rc != NO_ERROR) {
                    ALOGE("%s: set snapshot stream flip failed", __func__);
                }
            }
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : QCameraPicChannel
 *
 * DESCRIPTION: constructor of QCameraPicChannel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *
 * RETURN     : none
 *==========================================================================*/
QCameraPicChannel::QCameraPicChannel(uint32_t cam_handle,
                                     mm_camera_ops_t *cam_ops) :
    QCameraChannel(cam_handle, cam_ops)
{
    m_bAllowDynBufAlloc = true;
}

/*===========================================================================
 * FUNCTION   : QCameraPicChannel
 *
 * DESCRIPTION: default constructor of QCameraPicChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraPicChannel::QCameraPicChannel()
{
    m_bAllowDynBufAlloc = true;
}

/*===========================================================================
 * FUNCTION   : ~QCameraPicChannel
 *
 * DESCRIPTION: destructor of QCameraPicChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraPicChannel::~QCameraPicChannel()
{
}

/*===========================================================================
 * FUNCTION   : takePicture
 *
 * DESCRIPTION: send request for queued snapshot frames
 *
 * PARAMETERS :
 *   @buf : request buf info
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraPicChannel::takePicture (mm_camera_req_buf_t *buf)
{
    int32_t rc = m_camOps->request_super_buf(m_camHandle, m_handle, buf);
    return rc;
}

/*===========================================================================
 * FUNCTION   : cancelPicture
 *
 * DESCRIPTION: cancel request for queued snapshot frames
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraPicChannel::cancelPicture()
{
    int32_t rc = m_camOps->cancel_super_buf_request(m_camHandle, m_handle);
    return rc;
}

/*===========================================================================
 * FUNCTION   : stopAdvancedCapture
 *
 * DESCRIPTION: stop advanced capture based on advanced capture type.
 *
 * PARAMETERS :
 *   @type : advanced capture type.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraPicChannel::stopAdvancedCapture(mm_camera_advanced_capture_t type)
{
    int32_t rc = m_camOps->process_advanced_capture(m_camHandle,
            m_handle, type, 0, NULL);
    return rc;
}

/*===========================================================================
 * FUNCTION   : startAdvancedCapture
 *
 * DESCRIPTION: start advanced capture based on advanced capture type.
 *
 * PARAMETERS :
 *   @type : advanced capture type.
 *   @config: advance capture config
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraPicChannel::startAdvancedCapture(mm_camera_advanced_capture_t type,
        cam_capture_frame_config_t *config)
{
    int32_t rc = NO_ERROR;

    rc = m_camOps->process_advanced_capture(m_camHandle, m_handle, type,
            1, config);
    return rc;
}

/*===========================================================================
* FUNCTION   : flushSuperbuffer
 *
 * DESCRIPTION: flush the all superbuffer frames.
 *
 * PARAMETERS :
 *   @frame_idx : frame index of focused frame
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraPicChannel::flushSuperbuffer(uint32_t frame_idx)
{
    int32_t rc = m_camOps->flush_super_buf_queue(m_camHandle, m_handle, frame_idx);
    return rc;
}

/*===========================================================================
 * FUNCTION   : QCameraVideoChannel
 *
 * DESCRIPTION: constructor of QCameraVideoChannel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *
 * RETURN     : none
 *==========================================================================*/
QCameraVideoChannel::QCameraVideoChannel(uint32_t cam_handle,
                                         mm_camera_ops_t *cam_ops) :
    QCameraChannel(cam_handle, cam_ops)
{
}

/*===========================================================================
 * FUNCTION   : QCameraVideoChannel
 *
 * DESCRIPTION: default constructor of QCameraVideoChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraVideoChannel::QCameraVideoChannel()
{
}

/*===========================================================================
 * FUNCTION   : ~QCameraVideoChannel
 *
 * DESCRIPTION: destructor of QCameraVideoChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraVideoChannel::~QCameraVideoChannel()
{
}

/*===========================================================================
 * FUNCTION   : takePicture
 *
 * DESCRIPTION: send request for queued snapshot frames
 *
 * PARAMETERS :
 *   @mm_camera_req_buf_t : request buf info
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraVideoChannel::takePicture(mm_camera_req_buf_t *buf)
{
    int32_t rc = m_camOps->request_super_buf(m_camHandle, m_handle, buf);
    return rc;
}

/*===========================================================================
 * FUNCTION   : cancelPicture
 *
 * DESCRIPTION: cancel request for queued snapshot frames
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraVideoChannel::cancelPicture()
{
    int32_t rc = m_camOps->cancel_super_buf_request(m_camHandle, m_handle);
    return rc;
}

/*===========================================================================
 * FUNCTION   : releaseFrame
 *
 * DESCRIPTION: return video frame from app
 *
 * PARAMETERS :
 *   @opaque     : ptr to video frame to be returned
 *   @isMetaData : if frame is a metadata or real frame
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraVideoChannel::releaseFrame(const void * opaque, bool isMetaData)
{
    QCameraStream *pVideoStream = NULL;
    for (size_t i = 0; i < mStreams.size(); i++) {
        if (mStreams[i] != NULL && mStreams[i]->isTypeOf(CAM_STREAM_TYPE_VIDEO)) {
            pVideoStream = mStreams[i];
            break;
        }
    }

    if (NULL == pVideoStream) {
        ALOGE("%s: No video stream in the channel", __func__);
        return BAD_VALUE;
    }

    int32_t rc = pVideoStream->bufDone(opaque, isMetaData);
    return rc;
}

/*===========================================================================
 * FUNCTION   : QCameraReprocessChannel
 *
 * DESCRIPTION: constructor of QCameraReprocessChannel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *   @pp_mask    : post-proccess feature mask
 *
 * RETURN     : none
 *==========================================================================*/
QCameraReprocessChannel::QCameraReprocessChannel(uint32_t cam_handle,
                                                 mm_camera_ops_t *cam_ops) :
    QCameraChannel(cam_handle, cam_ops),
    m_pSrcChannel(NULL)
{
    memset(mSrcStreamHandles, 0, sizeof(mSrcStreamHandles));
}

/*===========================================================================
 * FUNCTION   : QCameraReprocessChannel
 *
 * DESCRIPTION: default constructor of QCameraReprocessChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraReprocessChannel::QCameraReprocessChannel() :
    m_pSrcChannel(NULL)
{
}

/*===========================================================================
 * FUNCTION   : ~QCameraReprocessChannel
 *
 * DESCRIPTION: destructor of QCameraReprocessChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCameraReprocessChannel::~QCameraReprocessChannel()
{
}

/*===========================================================================
 * FUNCTION   : addReprocStreamsFromSource
 *
 * DESCRIPTION: add reprocess streams from input source channel
 *
 * PARAMETERS :
 *   @allocator      : stream related buffer allocator
 *   @featureConfig  : pp feature configuration
 *   @pSrcChannel    : ptr to input source channel that needs reprocess
 *   @minStreamBufNum: number of stream buffers needed
 *   @burstNum       : number of burst captures needed
 *   @paddingInfo    : padding information
 *   @param          : reference to parameters
 *   @contStream     : continous streaming mode or burst
 *   @offline        : configure for offline reprocessing
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraReprocessChannel::addReprocStreamsFromSource(
        QCameraAllocator& allocator, cam_pp_feature_config_t &featureConfig,
        QCameraChannel *pSrcChannel, uint8_t minStreamBufNum, uint8_t burstNum,
        cam_padding_info_t *paddingInfo, QCameraParameters &param, bool contStream,
        bool offline)
{
    int32_t rc = 0;
    QCameraStream *pStream = NULL;
    QCameraHeapMemory *pStreamInfoBuf = NULL;
    QCameraHeapMemory *pMiscBuf = NULL;
    cam_stream_info_t *streamInfo = NULL;
    cam_padding_info_t padding;

    memset(mSrcStreamHandles, 0, sizeof(mSrcStreamHandles));
    if (NULL == paddingInfo) {
        return BAD_VALUE;
    }
    padding = *paddingInfo;
    //Use maximum padding so that the buffer
    //can be rotated
    padding.width_padding = MAX(padding.width_padding, padding.height_padding);
    padding.height_padding = padding.width_padding;

    CDBG("%s : %d: num of src stream = %d", __func__, __LINE__, pSrcChannel->getNumOfStreams());

    for (uint32_t i = 0; i < pSrcChannel->getNumOfStreams(); i++) {
        pStream = pSrcChannel->getStreamByIndex(i);
        if (pStream != NULL) {
            if (param.getofflineRAW() && !pStream->isTypeOf(CAM_STREAM_TYPE_RAW)) {
                //Skip all the stream other than RAW incase of offline of RAW
                continue;
            }
            if (pStream->isTypeOf(CAM_STREAM_TYPE_RAW) && !param.getofflineRAW()) {
                // Skip raw for reprocess now because PP module cannot handle
                // meta data&raw. May need furthur discussion if Imaginglib need meta data
                continue;
            }

            if ((pStream->isTypeOf(CAM_STREAM_TYPE_METADATA)) ||
                    (pStream->isTypeOf(CAM_STREAM_TYPE_ANALYSIS))) {
                // Skip metadata
                continue;
            }

            if (pStream->isTypeOf(CAM_STREAM_TYPE_PREVIEW) ||
                    pStream->isTypeOf(CAM_STREAM_TYPE_POSTVIEW) ||
                    pStream->isOrignalTypeOf(CAM_STREAM_TYPE_PREVIEW) ||
                    pStream->isOrignalTypeOf(CAM_STREAM_TYPE_POSTVIEW) ||
                    (param.getofflineRAW() && pStream->isTypeOf(CAM_STREAM_TYPE_RAW))) {
                uint32_t feature_mask = featureConfig.feature_mask;

                // skip thumbnail reprocessing if not needed
                if (!param.needThumbnailReprocess(&feature_mask)) {
                    continue;
                }
                // CAC, SHARPNESS, FLIP and WNR would have been already applied -
                // on preview/postview stream in realtime. Need not apply again.
                feature_mask &= ~(CAM_QCOM_FEATURE_DENOISE2D |
                        CAM_QCOM_FEATURE_CAC |
                        CAM_QCOM_FEATURE_SHARPNESS |
                        CAM_QCOM_FEATURE_FLIP);
                if (!feature_mask) {
                    // Skip thumbnail stream reprocessing since no other
                    //reprocessing is enabled.
                    continue;
                }
            }

            pStreamInfoBuf = allocator.allocateStreamInfoBuf(CAM_STREAM_TYPE_OFFLINE_PROC);
            if (pStreamInfoBuf == NULL) {
                ALOGE("%s: no mem for stream info buf", __func__);
                rc = NO_MEMORY;
                break;
            }

            streamInfo = (cam_stream_info_t *)pStreamInfoBuf->getPtr(0);
            memset(streamInfo, 0, sizeof(cam_stream_info_t));
            streamInfo->stream_type = CAM_STREAM_TYPE_OFFLINE_PROC;
            // Enable CPP high performance mode to put it in turbo frequency mode for
            // burst/longshot/HDR snapshot cases
            streamInfo->perf_mode = CAM_PERF_HIGH_PERFORMANCE;
            if (param.getofflineRAW() && pStream->isTypeOf(CAM_STREAM_TYPE_RAW)) {
                streamInfo->fmt = CAM_FORMAT_YUV_420_NV21;
            } else {
                rc = pStream->getFormat(streamInfo->fmt);
            }
            if (pStream->isTypeOf(CAM_STREAM_TYPE_POSTVIEW) ||
                    pStream->isTypeOf(CAM_STREAM_TYPE_PREVIEW)) {
                param.getThumbnailSize(&(streamInfo->dim.width), &(streamInfo->dim.height));
            } else {
                if ((param.getofflineRAW()) &&
                        (pStream->isTypeOf(CAM_STREAM_TYPE_RAW))) {
                    param.getStreamDimension(CAM_STREAM_TYPE_SNAPSHOT,streamInfo->dim);
                } else {
                    rc = pStream->getFrameDimension(streamInfo->dim);
                }
            }
            if ( contStream ) {
                streamInfo->streaming_mode = CAM_STREAMING_MODE_CONTINUOUS;
                streamInfo->num_of_burst = 0;
            } else {
                streamInfo->streaming_mode = CAM_STREAMING_MODE_BURST;
                streamInfo->num_of_burst = burstNum;
            }

            cam_stream_reproc_config_t rp_cfg;
            memset(&rp_cfg, 0, sizeof(cam_stream_reproc_config_t));
            if (offline) {
                cam_frame_len_offset_t offset;
                memset(&offset, 0, sizeof(cam_frame_len_offset_t));

                rp_cfg.pp_type = CAM_OFFLINE_REPROCESS_TYPE;
                pStream->getFormat(rp_cfg.offline.input_fmt);
                pStream->getFrameDimension(rp_cfg.offline.input_dim);
                pStream->getFrameOffset(offset);
                rp_cfg.offline.input_buf_planes.plane_info = offset;
                rp_cfg.offline.input_type = pStream->getMyOriginalType();
                //For input metadata + input buffer
                rp_cfg.offline.num_of_bufs = 2;
            } else {
                rp_cfg.pp_type = CAM_ONLINE_REPROCESS_TYPE;
                rp_cfg.online.input_stream_id = pStream->getMyServerID();
                rp_cfg.online.input_stream_type = pStream->getMyOriginalType();
            }
            param.getStreamRotation(streamInfo->stream_type,
                    streamInfo->pp_config, streamInfo->dim);
            streamInfo->reprocess_config = rp_cfg;
            streamInfo->reprocess_config.pp_feature_config = featureConfig;

            if (!(pStream->isTypeOf(CAM_STREAM_TYPE_SNAPSHOT) ||
                pStream->isOrignalTypeOf(CAM_STREAM_TYPE_SNAPSHOT))) {
                // CAC, SHARPNESS, FLIP and WNR would have been already applied -
                // on preview/postview stream in realtime. Need not apply again.
                streamInfo->reprocess_config.pp_feature_config.feature_mask &=
                        ~CAM_QCOM_FEATURE_CAC;
                streamInfo->reprocess_config.pp_feature_config.feature_mask &=
                        ~CAM_QCOM_FEATURE_SHARPNESS;
                streamInfo->reprocess_config.pp_feature_config.feature_mask &=
                        ~CAM_QCOM_FEATURE_FLIP;
                //Don't do WNR for thumbnail
                streamInfo->reprocess_config.pp_feature_config.feature_mask &=
                        ~CAM_QCOM_FEATURE_DENOISE2D;
                streamInfo->reprocess_config.pp_feature_config.feature_mask &=
                        ~CAM_QCOM_FEATURE_CDS;
                streamInfo->reprocess_config.pp_feature_config.feature_mask &=
                        ~CAM_QCOM_FEATURE_DSDN;

                if (param.isHDREnabled()
                  && !param.isHDRThumbnailProcessNeeded()){
                    streamInfo->reprocess_config.pp_feature_config.feature_mask
                      &= ~CAM_QCOM_FEATURE_HDR;
                }
            }

            cam_stream_type_t type = CAM_STREAM_TYPE_DEFAULT;
            if (offline) {
                type = streamInfo->reprocess_config.offline.input_type;
            } else {
                type = streamInfo->reprocess_config.online.input_stream_type;
            }
            if (type == CAM_STREAM_TYPE_SNAPSHOT) {
                int flipMode = param.getFlipMode(type);
                if (flipMode > 0) {
                    streamInfo->reprocess_config.pp_feature_config.feature_mask |=
                            CAM_QCOM_FEATURE_FLIP;
                    streamInfo->reprocess_config.pp_feature_config.flip = (uint32_t)flipMode;
                }
            }

            if ((streamInfo->reprocess_config.pp_feature_config.feature_mask
                    & CAM_QCOM_FEATURE_SCALE)
                    && param.m_reprocScaleParam.isScaleEnabled()
                    && param.m_reprocScaleParam.isUnderScaling()) {
                //we only Scale Snapshot frame
                if (pStream->isTypeOf(CAM_STREAM_TYPE_SNAPSHOT)) {
                    streamInfo->dim.width =
                            streamInfo->reprocess_config.pp_feature_config.scale_param.output_width;
                    streamInfo->dim.height =
                            streamInfo->reprocess_config.pp_feature_config.scale_param.output_height;
                }
                CDBG_HIGH("%s: stream width=%d, height=%d.",
                        __func__, streamInfo->dim.width, streamInfo->dim.height);
            }

            // save source stream handler
            mSrcStreamHandles[mStreams.size()] = pStream->getMyHandle();

            pMiscBuf = allocator.allocateMiscBuf(streamInfo);

            // add reprocess stream
            if (streamInfo->reprocess_config.pp_feature_config.feature_mask
                    & CAM_QCOM_FEATURE_ROTATION) {
                rc = addStream(allocator, pStreamInfoBuf, pMiscBuf,
                        minStreamBufNum, &padding, NULL, NULL, false, false,
                        streamInfo->reprocess_config.pp_feature_config.rotation);
            } else {
                rc = addStream(allocator, pStreamInfoBuf, pMiscBuf,
                        minStreamBufNum, &padding, NULL, NULL, false, false);
            }
            if (rc != NO_ERROR) {
                ALOGE("%s: add reprocess stream failed, ret = %d", __func__, rc);
                break;
            }
        }
    }

    if (rc == NO_ERROR) {
        m_pSrcChannel = pSrcChannel;
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : getStreamBySrouceHandle
 *
 * DESCRIPTION: find reprocess stream by its source stream handle
 *
 * PARAMETERS :
 *   @srcHandle : source stream handle
 *
 * RETURN     : ptr to reprocess stream if found. NULL if not found
 *==========================================================================*/
QCameraStream * QCameraReprocessChannel::getStreamBySrouceHandle(uint32_t srcHandle)
{
    QCameraStream *pStream = NULL;

    for (size_t i = 0; i < mStreams.size(); i++) {
        if (mSrcStreamHandles[i] == srcHandle) {
            pStream = mStreams[i];
            break;
        }
    }

    return pStream;
}

/*===========================================================================
 * FUNCTION   : stop
 *
 * DESCRIPTION: Unmap offline buffers and stop channel
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraReprocessChannel::stop()
{
    if (!mOfflineBuffers.empty()) {
        QCameraStream *stream = NULL;
        List<OfflineBuffer>::iterator it = mOfflineBuffers.begin();
        int error = NO_ERROR;
        for( ; it != mOfflineBuffers.end(); it++) {
            stream = (*it).stream;
            if (NULL != stream) {
                error = stream->unmapBuf((*it).type,
                                         (*it).index,
                                         -1);
                if (NO_ERROR != error) {
                    ALOGE("%s: Error during offline buffer unmap %d",
                          __func__, error);
                }
            }
        }
        mOfflineBuffers.clear();
    }

    return QCameraChannel::stop();
}

/*===========================================================================
 * FUNCTION   : doReprocessOffline
 *
 * DESCRIPTION: request to do offline reprocess on the frame
 *
 * PARAMETERS :
 *   @frame   : frame to be performed a reprocess
 *   @meta_buf : Metadata buffer for reprocessing
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraReprocessChannel::doReprocessOffline(mm_camera_super_buf_t *frame,
        mm_camera_buf_def_t *meta_buf)
{
    int32_t rc = 0;
    QCameraStream *pStream = NULL;

    if (mStreams.size() < 1) {
        ALOGE("%s: No reprocess streams", __func__);
        return -1;
    }
    if (m_pSrcChannel == NULL) {
        ALOGE("%s: No source channel for reprocess", __func__);
        return -1;
    }

    if (frame == NULL) {
        ALOGE("%s: Invalid source frame", __func__);
        return BAD_VALUE;
    }

    for (uint32_t i = 0; i < frame->num_bufs; i++) {
        pStream = getStreamBySrouceHandle(frame->bufs[i]->stream_id);
        if ((pStream != NULL) &&
                (m_handle == pStream->getChannelHandle())) {
            if (pStream->isTypeOf(CAM_STREAM_TYPE_METADATA)) {
                continue;
            }

            uint32_t meta_buf_index = 0;
            if (NULL != meta_buf) {
                rc = pStream->mapBuf(CAM_MAPPING_BUF_TYPE_OFFLINE_META_BUF,
                                     meta_buf_index,
                                     -1,
                                     meta_buf->fd,
                                     meta_buf->frame_len);
                if (NO_ERROR != rc ) {
                    ALOGE("%s : Error during metadata buffer mapping",
                          __func__);
                    break;
                }
                // we have meta data sent together with reprocess frame
                uint32_t stream_id = frame->bufs[i]->stream_id;
                QCameraStream *srcStream =
                        m_pSrcChannel->getStreamByHandle(stream_id);
                metadata_buffer_t *pMetaData =
                        (metadata_buffer_t *)meta_buf->buffer;
                if ((NULL != pMetaData) && (NULL != srcStream)) {
                    IF_META_AVAILABLE(cam_crop_data_t, crop, CAM_INTF_META_CROP_DATA, pMetaData) {
                        if (MAX_NUM_STREAMS > crop->num_of_streams) {
                            for (int j = 0; j < MAX_NUM_STREAMS; j++) {
                                if (crop->crop_info[j].stream_id ==
                                            srcStream->getMyServerID()) {
                                    // Store crop/roi information for offline reprocess
                                    // in the reprocess stream slot
                                    crop->crop_info[crop->num_of_streams].crop =
                                            crop->crop_info[j].crop;
                                    crop->crop_info[crop->num_of_streams].roi_map =
                                            crop->crop_info[j].roi_map;
                                    for (uint8_t k = 0; k < mStreams.size(); k++) {
                                        if (srcStream->getMyType() ==
                                                mStreams[k]->getMyOriginalType()) {
                                            crop->crop_info[crop->num_of_streams].stream_id =
                                                    mStreams[k]->getMyServerID();
                                            break;
                                        }
                                    }
                                    crop->num_of_streams++;
                                    break;
                                }
                            }
                        } else {
                            ALOGE("%s: No space to add reprocess stream crop/roi information",
                                    __func__);
                        }
                    }
                }
            }
            mappedBuffer.index = meta_buf_index;
            mappedBuffer.stream = pStream;
            mappedBuffer.type = CAM_MAPPING_BUF_TYPE_OFFLINE_META_BUF;
            mOfflineBuffers.push_back(mappedBuffer);

            uint32_t buf_index = 1;
            rc = pStream->mapBuf(CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF,
                                 buf_index,
                                 -1,
                                 frame->bufs[i]->fd,
                                 frame->bufs[i]->frame_len);
            if (NO_ERROR != rc ) {
                ALOGE("%s : Error during reprocess input buffer mapping",
                      __func__);
                break;
            }
            mappedBuffer.index = buf_index;
            mappedBuffer.stream = pStream;
            mappedBuffer.type = CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF;
            mOfflineBuffers.push_back(mappedBuffer);

            cam_stream_parm_buffer_t param;
            memset(&param, 0, sizeof(cam_stream_parm_buffer_t));

            param.type = CAM_STREAM_PARAM_TYPE_DO_REPROCESS;
            param.reprocess.buf_index = buf_index;
            param.reprocess.frame_idx = frame->bufs[i]->frame_idx;
            param.reprocess.meta_present = 1;
            param.reprocess.meta_buf_index = meta_buf_index;

            rc = pStream->setParameter(param);
            if (rc != NO_ERROR) {
                ALOGE("%s: stream setParameter for reprocess failed",
                      __func__);
                break;
            }
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : doReprocess
 *
 * DESCRIPTION: request to do a reprocess on the frame
 *
 * PARAMETERS :
 *   @frame   : frame to be performed a reprocess
 *   @mParameter : camera parameters
 *   @pMetaStream: Metadata stream handle
 *   @meta_buf_index : Metadata buffer index
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraReprocessChannel::doReprocess(mm_camera_super_buf_t *frame,
        QCameraParameters &mParameter, QCameraStream *pMetaStream,
        uint8_t meta_buf_index)
{
    int32_t rc = 0;
    if (mStreams.size() < 1) {
        ALOGE("%s: No reprocess streams", __func__);
        return -1;
    }
    if (m_pSrcChannel == NULL) {
        ALOGE("%s: No source channel for reprocess", __func__);
        return -1;
    }

    for (uint32_t i = 0; i < frame->num_bufs; i++) {
        QCameraStream *pStream = getStreamBySrouceHandle(frame->bufs[i]->stream_id);
        if ((pStream != NULL) && (m_handle == pStream->getChannelHandle())) {
            if (mParameter.getofflineRAW() &&
                    !pStream->isOrignalTypeOf(CAM_STREAM_TYPE_RAW)) {
                continue;
            }
            if ((pStream->isTypeOf(CAM_STREAM_TYPE_METADATA)) ||
                    (pStream->isTypeOf(CAM_STREAM_TYPE_ANALYSIS))) {
                // Skip metadata for reprocess now because PP module cannot handle meta data
                // May need furthur discussion if Imaginglib need meta data
                continue;
            }

            cam_stream_parm_buffer_t param;
            memset(&param, 0, sizeof(cam_stream_parm_buffer_t));
            param.type = CAM_STREAM_PARAM_TYPE_DO_REPROCESS;
            param.reprocess.buf_index = frame->bufs[i]->buf_idx;
            param.reprocess.frame_idx = frame->bufs[i]->frame_idx;
            if (pMetaStream != NULL) {
                // we have meta data frame bundled, sent together with reprocess frame
                param.reprocess.meta_present = 1;
                param.reprocess.meta_stream_handle = pMetaStream->getMyServerID();
                param.reprocess.meta_buf_index = meta_buf_index;
            }

            CDBG_HIGH("Frame for reprocessing id = %d buf Id = %d meta index = %d",
                    param.reprocess.frame_idx, param.reprocess.buf_index,
                    param.reprocess.meta_buf_index);

            rc = pStream->setParameter(param);
            if (rc != NO_ERROR) {
                ALOGE("%s: stream setParameter for reprocess failed", __func__);
                break;
            }
        }
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : doReprocess
 *
 * DESCRIPTION: request to do a reprocess on the frame
 *
 * PARAMETERS :
 *   @buf_fd     : fd to the input buffer that needs reprocess
 *   @buf_lenght : length of the input buffer
 *   @ret_val    : result of reprocess.
 *                 Example: Could be faceID in case of register face image.
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraReprocessChannel::doReprocess(int buf_fd,
        size_t buf_length, int32_t &ret_val)
{
    int32_t rc = 0;
    if (mStreams.size() < 1) {
        ALOGE("%s: No reprocess streams", __func__);
        return -1;
    }

    uint32_t buf_idx = 0;
    for (size_t i = 0; i < mStreams.size(); i++) {
        if ((mStreams[i] != NULL) &&
                (m_handle != mStreams[i]->getChannelHandle())) {
            continue;
        }
        rc = mStreams[i]->mapBuf(CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF,
                                 buf_idx, -1,
                                 buf_fd, buf_length);

        if (rc == NO_ERROR) {
            cam_stream_parm_buffer_t param;
            memset(&param, 0, sizeof(cam_stream_parm_buffer_t));
            param.type = CAM_STREAM_PARAM_TYPE_DO_REPROCESS;
            param.reprocess.buf_index = buf_idx;
            rc = mStreams[i]->setParameter(param);
            if (rc == NO_ERROR) {
                ret_val = param.reprocess.ret_val;
            }
            mStreams[i]->unmapBuf(CAM_MAPPING_BUF_TYPE_OFFLINE_INPUT_BUF,
                                  buf_idx, -1);
        }
    }
    return rc;
}

}; // namespace qcamera
