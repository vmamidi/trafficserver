/** @file
 *
 *  A brief file description
 *
 *  @section license License
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "QUICStream.h"
#include "QUICBidirectionalStream.h"
#include "QUICUnidirectionalStream.h"
#include "QUICStreamFactory.h"

ClassAllocator<QUICBidirectionalStream> quicBidiStreamAllocator("quicBidiStreamAllocator");
ClassAllocator<QUICSendStream> quicSendStreamAllocator("quicSendStreamAllocator");
ClassAllocator<QUICReceiveStream> quicReceiveStreamAllocator("quicReceiveStreamAllocator");

QUICStreamVConnection *
QUICStreamFactory::create(QUICStreamId sid, uint64_t local_max_stream_data, uint64_t remote_max_stream_data)
{
  QUICStreamVConnection *stream = nullptr;
  switch (QUICTypeUtil::detect_stream_direction(sid, this->_info->direction())) {
  case QUICStreamDirection::BIDIRECTIONAL:
    // TODO Free the stream somewhere
    stream = THREAD_ALLOC(quicBidiStreamAllocator, this_ethread());
    new (stream) QUICBidirectionalStream(this->_rtt_provider, this->_info, sid, local_max_stream_data, remote_max_stream_data);
    break;
  case QUICStreamDirection::SEND:
    // TODO Free the stream somewhere
    stream = THREAD_ALLOC(quicSendStreamAllocator, this_ethread());
    new (stream) QUICSendStream(this->_info, sid, remote_max_stream_data);
    break;
  case QUICStreamDirection::RECEIVE:
    // server side
    // TODO Free the stream somewhere
    stream = THREAD_ALLOC(quicReceiveStreamAllocator, this_ethread());
    new (stream) QUICReceiveStream(this->_rtt_provider, this->_info, sid, local_max_stream_data);
    break;
  default:
    ink_assert(false);
    break;
  }

  return stream;
}

void
QUICStreamFactory::delete_stream(QUICStreamVConnection *stream)
{
  if (!stream) {
    return;
  }

  stream->~QUICStreamVConnection();
  switch (stream->direction()) {
  case QUICStreamDirection::BIDIRECTIONAL:
    THREAD_FREE(static_cast<QUICBidirectionalStream *>(stream), quicBidiStreamAllocator, this_thread());
    break;
  case QUICStreamDirection::SEND:
    THREAD_FREE(static_cast<QUICSendStream *>(stream), quicSendStreamAllocator, this_thread());
    break;
  case QUICStreamDirection::RECEIVE:
    THREAD_FREE(static_cast<QUICReceiveStream *>(stream), quicReceiveStreamAllocator, this_thread());
    break;
  default:
    ink_assert(false);
    break;
  }
}
