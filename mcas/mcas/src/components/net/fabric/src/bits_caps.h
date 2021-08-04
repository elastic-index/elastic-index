/*
   Copyright [2017-2019] [IBM Corporation]
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
       http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


/* Primary capabilities */

X(FI_MSG) /* sending and receiving messages or datagrams */
X(FI_RMA) /* RMA read and write operations */
X(FI_TAGGED) /* tagged message transfers */
X(FI_ATOMIC) /* some set of atomic operations */
X(FI_MULTICAST) /* multicast data transfers */
X(FI_NAMED_RX_CTX) /* allow an initiator to target a specific receive context */
X(FI_DIRECTED_RECV) /* use the source address of an incoming message when matching it with a receive buffer */
X(FI_READ) /*  capable of initiating reads against remote memory regions */
X(FI_WRITE) /* capable of initiating writes against remote memory regions */
X(FI_RECV) /* capable of receiving message data transfers */
X(FI_SEND) /* capable of sending message data transfers */
X(FI_REMOTE_READ) /* capable of receiving read memory operations from remote endpoints */
X(FI_REMOTE_WRITE) /* capable of receiving write memory operations from remote endpoints */
X(FI_VARIABLE_MSG)

/* Other capabilities */

X(FI_MULTI_RECV) /* must support the FI_MULTI_RECV flag */
X(FI_REMOTE_CQ_DATA)
X(FI_MORE)
X(FI_PEEK)
X(FI_TRIGGER) /* support triggered operations */
X(FI_FENCE) /* support the FI_FENCE flag */

X(FI_COMPLETION)
X(FI_INJECT)
X(FI_INJECT_COMPLETE)
X(FI_TRANSMIT_COMPLETE)
X(FI_DELIVERY_COMPLETE)
X(FI_AFFINITY)
X(FI_COMMIT_COMPLETE)
X(FI_MATCH_COMPLETE)

X(FI_HMEM)
X(FI_VARIABLE_MSG)
X(FI_RMA_PMEM) /* provider is 'persistent memory aware' */
X(FI_SOURCE_ERR) /* raw source addressing data be returned as part of completion data for any addressing error */
X(FI_LOCAL_COMM) /* support host local communication */
X(FI_REMOTE_COMM) /* support remote communication */
X(FI_SHARED_AV) /* support for shared address vectors */
X(FI_PROV_ATTR_ONLY)
X(FI_NUMERICHOST)
X(FI_RMA_EVENT) /* support the generation of completion events when it is the target of an RMA or atomic operation */
X(FI_SOURCE) /* return source addressing data as part of its completion data */
X(FI_NAMED_RX_CTX)
X(FI_DIRECTED_RECV)
