/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2018, Live Networks, Inc.  All rights reserved
// LIVE555 Media Server
// main program

#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "DynamicRTSPServer.hh"
#include "version.hh"

char g_listenAddr[129] = "\0";
int g_listenPort = 554;


void showUsage(void)
{
	printf("Usage:\r\n");
	printf("mediaServerEx [-listen_addr <ip>] [-listen_port <tcp port>] [-h]");
	printf("\t-listen_addr\tIf multiple NICs present, listen on the one given, otherwise live555 decides based on Multicast routing (see docs).\r\n");
	printf("\t-listen_port\tTCP Port on which the RTSP server is listening, default is 554\r\n");
	printf("\t-h          \tShow this help\r\n");
}

int parseArgs(int argc, char** argv)
{
	if (argc == 1)
	{
		printf("Starting with default values. Add '-h' for additional help...");
		return 0;
	}

	for (int i = 1; i < argc; i++) 
	{
		if (argv[i] == NULL) break;

		if (_stricmp("-listen_addr", argv[i]) == 0)
		{
			if (argv[++i] != NULL) strcpy_s(g_listenAddr, argv[i]);
			else break;
		}
		if (_stricmp("-listen_port", argv[i]) == 0)
		{
			if (argv[++i] != NULL) g_listenPort = atoi(argv[i]);
			else break;
		}

		if (_stricmp("-h", argv[i]) == 0)
		{
			showUsage();
			return 1;
		}
	}

	return 0;
}

int main(int argc, char** argv) 
{
	// failed to parse args or show help
	if (parseArgs(argc, argv) != 0) return 1;

	if (strlen(g_listenAddr) > 0) ReceivingInterfaceAddr = SendingInterfaceAddr = our_inet_addr(g_listenAddr);
	portNumBits rtspServerPortNum = g_listenPort;

	RTSPServer* rtspServer = NULL;
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);
	UserAuthenticationDatabase* authDB = NULL;

#ifdef ACCESS_CONTROL
	// To implement client access control to the RTSP server, do the following:
	authDB = new UserAuthenticationDatabase;
	authDB->addUserRecord("username1", "password1"); // replace these with real strings
	// Repeat the above with each <username>, <password> that you wish to allow
	// access to the server.
#endif

	// Create the RTSP server.
	rtspServer = DynamicRTSPServer::createNew(*env, rtspServerPortNum, authDB);
	if (rtspServer == NULL) 
	{
		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
		return 10;
	}

	*env << "LIVE555 Media Server\n";
	*env << "\tversion " << MEDIA_SERVER_VERSION_STRING
		<< " (LIVE555 Streaming Media library version "
		<< LIVEMEDIA_LIBRARY_VERSION_STRING << ").\n";

	char* urlPrefix = rtspServer->rtspURLPrefix();
	*env << "Play streams from this server using the URL\n\t"
		<< urlPrefix << "<filename>\nwhere <filename> is a file present in the current directory.\n";
	*env << "Each file's type is inferred from its name suffix:\n";
	*env << "\t\".264\" => a H.264 Video Elementary Stream file\n";
	*env << "\t\".265\" => a H.265 Video Elementary Stream file\n";
	*env << "\t\".aac\" => an AAC Audio (ADTS format) file\n";
	*env << "\t\".ac3\" => an AC-3 Audio file\n";
	*env << "\t\".amr\" => an AMR Audio file\n";
	*env << "\t\".dv\" => a DV Video file\n";
	*env << "\t\".m4e\" => a MPEG-4 Video Elementary Stream file\n";
	*env << "\t\".mkv\" => a Matroska audio+video+(optional)subtitles file\n";
	*env << "\t\".mp3\" => a MPEG-1 or 2 Audio file\n";
	*env << "\t\".mpg\" => a MPEG-1 or 2 Program Stream (audio+video) file\n";
	*env << "\t\".ogg\" or \".ogv\" or \".opus\" => an Ogg audio and/or video file\n";
	*env << "\t\".ts\" => a MPEG Transport Stream file\n";
	*env << "\t\t(a \".tsx\" index file - if present - provides server 'trick play' support)\n";
	*env << "\t\".vob\" => a VOB (MPEG-2 video with AC-3 audio) file\n";
	*env << "\t\".wav\" => a WAV Audio file\n";
	*env << "\t\".webm\" => a WebM audio(Vorbis)+video(VP8) file\n";
	*env << "See http://www.live555.com/mediaServer/ for additional documentation.\n";

	// Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
	// Try first with the default HTTP port (80), and then with the alternative HTTP
	// port numbers (8000 and 8080).

	if (rtspServer->setUpTunnelingOverHTTP(80) || 
		rtspServer->setUpTunnelingOverHTTP(8000) || 
		rtspServer->setUpTunnelingOverHTTP(8080)) 
	{
		*env << "(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n";
	}
	else 
	{
		*env << "(RTSP-over-HTTP tunneling is not available.)\n";
	}

	env->taskScheduler().doEventLoop(); // does not return

	return 0; // only to prevent compiler warning
}
