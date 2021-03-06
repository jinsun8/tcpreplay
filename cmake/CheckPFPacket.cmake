###################################################################
#  $Id$
#
#  Copyright (c) 2009 Aaron Turner, <aturner at synfin dot net>
#  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in
#   the documentation and/or other materials provided with the
#   distribution.
#
# * Neither the name of the Aaron Turner nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
###################################################################
# - Find out if the system supports Linux's PF_PACKET socket API
# we only try compiling the test since that looks for PF_PACKET


INCLUDE(CheckCSourceRuns)
      
CHECK_C_SOURCE_RUNS("
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>     /* the L2 protocols */
#include <netinet/in.h>       /* htons */
#include <stdlib.h>


int
main(int argc, char *argv[])
{
	int pf_socket;
	pf_socket = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL));
    exit(0);
}
"
    PFPACKET_RUN_RESULT)

SET(HAVE_PF_PACKET NO)
IF(PFPACKET_RUN_RESULT EQUAL 1)
    SET(HAVE_PF_PACKET YES)
    MESSAGE(STATUS "System has PF_PACKET sockets")
ELSE(PFPACKET_RUN_RESULT EQUAL 1)
    MESSAGE(STATUS "System does not have PF_PACKET sockets")
ENDIF(PFPACKET_RUN_RESULT EQUAL 1)
