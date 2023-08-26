
#include "precompiled.h"
#include "main.h"
#include "resource.h"
#include "secureSocket.h"
#include "unsecureSocket.h"
#include "productKey.h"
#include "utilityClock.h"

const float Webserver::SessionTimeout = 60 * 5.0f;

void Webserver::Initialize( void )
{
   WSADATA wsaData;

   int result = StartSSL( );
   if ( result <= 0 ) 
   {
      g_Log.Write( Log::TypeError, "Failed to start SSL, code %d", result );
      return;
   }

   m_ListenSocket  = INVALID_SOCKET;
   m_StartupTimer  = 0.0f;

   m_pUpdateString = (char *) malloc( 1024 );
   m_pUpdateString[ 0 ] = NULL;

   WSAStartup( MAKEWORD( 2, 2 ), &wsaData );

   KeyRand::Seed( (short) GetTickCount( ) );

   ResourceDesc desc;

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_HEADER_HTML), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "header.html", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_SECUREHEADER_HTML), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "secureHeader.html", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_HOME_HTML), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "home.html", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_FOOTER_HTML), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "footer.html", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_HEADER_PNG), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "header.png", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_ICON_GIF), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "icon.gif", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_A_GIF), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "a.gif", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_B_GIF), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "b.gif", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_C_JPG), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "c.jpg", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_BG_JPG), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "bg.jpg", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_OVAL_BLUE_LEFT_GIF), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "oval-blue-left.gif", desc );

   desc.hInfo  = FindResource( GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_OVAL_BLUE_RIGHT_GIF), "BINARY" );
   desc.hResource = LoadResource( GetModuleHandle(NULL), desc.hInfo );
   m_ResourceHash.Add( "oval-blue-right.gif", desc );

   m_NeedsRestart = FALSE;
}

void Webserver::Startup( void )
{   
   if ( Timer::m_total < m_StartupTimer ) return;

   //try every 10 seconds
   m_StartupTimer = Timer::m_total + 10.0f;

   //create a socket to open in the outside world
   m_ListenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
   
   sockaddr_in service;

   service.sin_family      = AF_INET;
   service.sin_addr.s_addr = INADDR_ANY;
   service.sin_port        = htons( g_Options.m_Options.port );

   int result = bind( m_ListenSocket, (SOCKADDR*) &service, sizeof(service) );

   //if it fails attempt to bind on the default port
   if ( SOCKET_ERROR == result )
   {
      static int printOnce;

      //don't spam the file incase it can't infinitely can't connect
      if ( 0 == printOnce )
      {
         g_Log.Write( Log::TypeWarning, "Failed to bind to port: %d, defaulting to port: %d", g_Options.m_Options.port, OptionsControl::DefaultWebPort );
         printOnce = 1;
      }

      service.sin_port = htons( OptionsControl::DefaultWebPort );
      result = bind( m_ListenSocket, (SOCKADDR*) &service, sizeof(service) );
   }

   if ( SOCKET_ERROR == result )
   {
      static int printOnce;
      
      //don't spam the file incase it can't infinitely can't connect
      if ( 0 == printOnce )
      {
         g_Log.Write( Log::TypeError, "Failed to bind to any port, the webserver will keep trying..." );
         printOnce = 1;
      }

      shutdown( m_ListenSocket, SD_BOTH );
      closesocket( m_ListenSocket );

      m_ListenSocket = INVALID_SOCKET;
      return;
   }

   result = listen( m_ListenSocket, SOMAXCONN );

   if ( SOCKET_ERROR == result )
   {
      g_Log.Write( Log::TypeError, "Listen socket failed" );

      shutdown( m_ListenSocket, SD_BOTH );
      closesocket( m_ListenSocket );

      m_ListenSocket = INVALID_SOCKET;
      return;
   }

   //set to non blocking
   u_long nonBlocking = 1;
   ioctlsocket( m_ListenSocket, FIONBIO, &nonBlocking );
   
   m_EmailTimer = 0.0f;

   g_Log.Write( Log::TypeInfo, "Webserver Initialized" );

   m_SmtpSender.Create( );
}

void Webserver::Uninitialize( void )
{
   uint32 i;

   Enumerator<const char *, ResourceDesc> e = m_ResourceHash.GetEnumerator( );
   
   while ( e.EnumNext( ) )
   {
      FreeResource( e.Data( ).hResource );
   }

   m_ResourceHash.Clear( );

   m_SmtpSender.Destroy( );

   for ( i = 0; i < MaxConnections; i++ )
   {
      m_Sessions[ i ].Destroy( );
   }

   shutdown( m_ListenSocket, SD_BOTH );
   closesocket( m_ListenSocket );

   m_ListenSocket = INVALID_SOCKET;

   free( m_pUpdateString );

   WSACleanup( );
   
   ShutdownSSL( );
}

void Webserver::Update( void )
{
   if ( INVALID_SOCKET == m_ListenSocket )
   {
      Startup( );
      return;
   }
   
   sockaddr_in address;
   SOCKET socket;

   uint32 i;

   int length = sizeof(address);

   if ( TRUE == m_NeedsRestart )
   {
      Uninitialize( );
      Initialize( );
   }

   socket = WSAAccept( m_ListenSocket, (sockaddr *) &address, &length, NULL, NULL );

   if ( INVALID_SOCKET != socket )
   {
      //see if this address is already in our session cache
      for ( i = 0; i < MaxConnections; i++ )
      {
         if ( TRUE == m_Sessions[ i ].IsOpen( ) )
         {
            if ( TRUE == m_Sessions[ i ].ValidateAddress( &address.sin_addr ) )
            {
               break;
            }
         }
      }
      
      //if the address wasn't found, find an expired session and use
      //that for our new session
      if ( i == MaxConnections )
      {
         for ( i = 0; i < MaxConnections; i++ )
         {
            if ( FALSE == m_Sessions[ i ].IsOpen( ) )
            {
               m_Sessions[ i ].Destroy( );
               m_Sessions[ i ].Create( SessionTimeout );
               break;
            }
         }
      }

      //regardless if we found an existing session or created a new session we
      //want to copy the new socket over
      if ( i < MaxConnections )
      {
         Socket  *pSocket;
         hostent *pHost = gethostbyname( "localhost" ); 
         
         if ( 0 == memcmp(pHost->h_addr_list[ 0 ], &address.sin_addr, sizeof(address.sin_addr)) )
         {
            pSocket = new UnsecureSocket;
            pSocket->Create( socket, &address );

            m_Sessions[ i ].AddSocket( pSocket );
         }
         else if ( 0 != g_Options.m_Options.allowRemoteAccess )
         {
            pSocket = new SecureSocket;
            pSocket->Create( socket, &address );
            ((SecureSocket *)pSocket)->SetSSL( SSL_new(m_SSLDesc.pCtx) );

            m_Sessions[ i ].AddSocket( pSocket );
         }
         else
         {
            //we don't allow WAN connections
            shutdown( socket, SD_BOTH );
            closesocket( socket );
         }
      }
   }

   //update any valid sockets so they
   //can handle requests, finish sending data, etc.
   for ( i = 0; i < MaxConnections; i++ )
   {
      UpdateConnection( &m_Sessions[ i ] );
   }

   if ( Timer::m_total > m_EmailTimer )
   {
      SendHourlyEmail( );
      
      //one hour from now
      m_EmailTimer = Timer::m_total + 60 * 60.0f;
   }
}

void Webserver::LaunchWebsite( void )
{
   char url[ 256 ] = { 0 };
   _snprintf( url, sizeof(url) - 1, "http://localhost:%d", g_Options.m_Options.port );

   ShellExecute( NULL, "open", url, NULL, NULL, SW_SHOWNORMAL );
}

void Webserver::UpdateConnection(
   Session *pSession
)
{
   Socket *pSocket;

   pSession->ResetSocketEnum( );

   while ( NULL != (pSocket = pSession->GetNextSocket( )) )
   {
      pSession->SetActiveSocket( pSocket );

      if ( pSession->ReadHTTP( ) )
      {
         HandleRequest( pSession );
      }
   }
}

void Webserver::HandleRequest(
   Session *pSession
)
{
   //first check if it's a dynamic page
   //(image, .html, etc.) those are generic
   //and can always be sent
   //UNLESS they come from our database "thumb., pic."
   
   //if it's not from our database
   //then attempt to send it as a dynamic page
   if ( 0 == strstr(pSession->RequestedPage( ), "thumb.") &&
        0 == strstr(pSession->RequestedPage( ), "pic.") )
   {
      if ( TRUE == SendDynamicPage(pSession) )
      {
         return;
      }
   }

   //user is always allowed to login,
   //even if the app is expired he might need to shut down
   if ( 0 == strcmp("doLogin", pSession->RequestedPage()) )
   {
      DoLogin( pSession );
   }

   if ( 0 == strcmp("doEmailLostPassword", pSession->RequestedPage()) )
   {
      DoEmailLostPassword( pSession );
      return;
   }

   int daysToExpire = GetDaysToExpire( );

   if ( daysToExpire >= 0 )
   {
      if ( 0 == strcmp("generateKey", pSession->RequestedPage()) )
      {
         SendGenerateKeyPage( pSession );
         return;
      }
      
      if ( 0 == strcmp("enterKey", pSession->RequestedPage()) )
      {
         SendEnterKeyPage( pSession );
         return;
      }
      
      if ( 0 == strcmp("doEnterKey", pSession->RequestedPage()) )
      {
         DoEnterKey( pSession );
         return;
      }
   }
   
   //secure means logged in, valid id for the link clicked and hasn't expired
   if ( TRUE == pSession->IsSecure( ) )
   {
      if ( 0 == strcmp("home", pSession->RequestedPage( )) )
      {
         SendHomePage( pSession );
      }
      else if ( 0 == strcmp("doClearLogs", pSession->RequestedPage( )) )
      {
         g_Log.Clear( );
         g_Log.Write( Log::TypeInfo, "Logs Cleared" );
         SendDiagnosticPage( pSession );
      }
      else if ( 0 == strcmp("diagnostic", pSession->RequestedPage( )) )
      {
         SendDiagnosticPage( pSession );
      }
      else if ( 0 == strcmp("shutdown", pSession->RequestedPage( )) )
      {
         SendShutdownPage( pSession );
      }
      else if ( 0 == strcmp("doShutdown", pSession->RequestedPage( )) )
      {
         DoShutdown( pSession );
      }
      else if ( 0 == strcmp("signOut", pSession->RequestedPage( )) )
      {
         pSession->LogOut( );
         SendLoginPage( pSession );
      }
      else if ( 0 != daysToExpire )
      {
         //all these options are only available if the app has not expired

         if ( 0 == strcmp("serverOptions", pSession->RequestedPage( )) )
         {
            SendServerOptionsPage( pSession );
         }
         else if ( 0 == strcmp("configureEmail", pSession->RequestedPage( )) )
         {
            SendConfigureEmailPage( pSession );
         }
         else if ( 0 == strcmp("configurePictures", pSession->RequestedPage( )) )
         {
            SendConfigurePicturesPage( pSession );
         }
         else if ( 0 == strcmp("doServerOptions", pSession->RequestedPage( )) )
         {
            DoServerOptions( pSession );
         }
         else if ( 0 == strcmp("doConfigureEmail", pSession->RequestedPage( )) )
         {
            DoConfigureEmail( pSession );
         }
         else if ( 0 == strcmp("doConfigurePictures", pSession->RequestedPage( )) )
         {
            DoConfigurePictures( pSession );
         }
         else if ( strstr(pSession->RequestedPage( ), "day.") )
         {
            SendDayPage( pSession );
         }
         else if ( strstr(pSession->RequestedPage( ), "album.") )
         {
            SendAlbumPage( pSession );
         }
         else if ( strstr(pSession->RequestedPage( ), "thumb.") )
         {
            SendThumbnail( pSession );
         }
         else if ( strstr(pSession->RequestedPage( ), "pic.") )
         {
            SendImage( pSession );
         }
         else if ( strstr(pSession->RequestedPage( ), "confirmClear.") )
         {
            SendConfirmClearPicturesPage( pSession );
         }
         else if ( strstr(pSession->RequestedPage( ), "clear.") )
         {
            DoClearPictures( pSession );
         }
         else
         {
            SendHomePage( pSession );
         }
      }
      else
      {
         SendHomePage( pSession );
      }
   }
   else 
   {  
      SendLoginPage( pSession );
   }
}

BOOL Webserver::SendDynamicPage(
   Session *pSession
)
{
   const char *pHtmlType = NULL;
   const char *pType = strrchr( pSession->RequestedPage( ), '.' );

   if ( pType )
   {
      if ( 0 == strcmp(pType, ".png") )
      {
         pHtmlType = "image/png";
      }
      else if ( 0 == strcmp(pType, ".bmp") )
      {
         pHtmlType = "image/bmp";
      }
      else if ( 0 == strcmp(pType, ".gif") )
      {
         pHtmlType = "image/gif";
      }
      else if ( 0 == strcmp(pType, ".jpeg") )
      {
         pHtmlType = "image/jpeg";
      }
      else if ( 0 == strcmp(pType, ".jpg") )
      {
         pHtmlType = "image/jpeg";
      }
      else if ( 0 == strcmp(pType, ".html") )
      {
         pHtmlType = "text/html";
      }
   }

   if ( NULL != pHtmlType )
   {
      pSession->BeginSendHTTP( pHtmlType );
         SendResource( pSession->RequestedPage(), pSession );
      pSession->EndSendHTTP( );
      
      return TRUE;
   }
   
   return FALSE;
}

void Webserver::SendServerOptionsPage(
   Session *pSession
)
{
   //copy our current configurations settings
   //into the config page and send it to the user
   char customOptions[ 2048 ] = { 0 };

   BuildServerOptionsPage( customOptions, sizeof(customOptions) );

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( customOptions );
      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendConfigurePicturesPage(
   Session *pSession
)
{
   //copy our current configurations settings
   //into the config page and send it to the user
   char *pPictures = new char[ 1024 * 10 ];

   BuildConfigurePicturesPage( pPictures, 1024 * 10 );

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( pPictures );
      SendFooter( pSession );

   pSession->EndSendHTTP( );

   delete [] pPictures;
}

void Webserver::SendConfigureEmailPage(
   Session *pSession
)
{
   //copy our current email configurations settings
   //into the config page and send it to the user
   char customEmail[ 2048 ] = { 0 };

   BuildConfigureEmailPage( customEmail, sizeof(customEmail) );

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( customEmail );
      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendExpiredPage(
   Session *pSession
)
{
   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendEnterKeyPage(
   Session *pSession
)
{
   const char enterKey[] =
      "<form method=\"post\" action=\"doEnterKey?<sessionId>\">"
      "<table>"
      "<tr><td class =\"pb\" align=\"left\">Product Key:</td><td class =\"pb\"><input type = \"text\" name = \"key\" value = \"\" maxlength = 16 size = 20 ></td></tr>"
      "<tr><td class =\"pb\" align=\"left\" colspan = 2><input type = \"submit\" name = \"submit\" value = \"Submit\"></td></tr>"
      "</table>"
      "</form>"
      "<a href = \"home?<sessionId>\">Home</a>";

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( enterKey );
      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendGenerateKeyPage(
   Session *pSession
)
{
   ProductKey::Key key;

   char text[ 256 ] = { 0 };

   const char generateKey[] =
      "<table>"
      "<tr><td class =\"pb\" align=\"left\">Key: <b>%s</b></td></tr>"
      "</table>";
      "<a href = \"home?<sessionId>\">Home</a>";

   ProductKey::GenerateKey( &key, 0 );

   _snprintf( text, sizeof(text) - 1, generateKey, key.key );

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( text );
      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendDiagnosticPage(
   Session *pSession
)
{
   const Log::Message *pMessage;

   uint32 index = 0;

   char text[ 512 ] = { 0 };

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      
      //send port, product key information
      const char generateKey[] =
         "<table>"
         "<tr><td class =\"pb\" align=\"left\">Port: <b>%d</b> | Product Id: <b>%d</b></td></tr>"
         "</table>";

      int productId;

      if ( FALSE == ProductKey::VerifyKey(&g_Options.m_Options.productKey, &productId) )
      {
         productId = 0;
      }

      _snprintf( text, sizeof(text) - 1, generateKey, g_Options.m_Options.port, productId );
      pSession->SendHTTP( text );
      

      //header for logs
      pSession->SendHTTP( "" );
      pSession->SendHTTP( "<table width = 800, heigt = 600>" );

      _snprintf( text, sizeof(text) - 1, "<tr><td class =\"pb\" align = \"left\"><b>%s Log</b> - <a href = \"diagnostic?<sessionId>\">Refresh</a></td></tr>", OptionsControl::AppName );
      pSession->SendHTTP( text );

      pSession->SendHTTP( "<tr><td class =\"pb\" align = \"left\"></td></tr>" );
      pSession->SendHTTP( "<tr><td class =\"pb\" align = \"left\">" );

      //send each log
      while ( NULL != (pMessage = g_Log.GetMesssage(index)) )
      {  
         if ( Log::TypeError == pMessage->type )
         {
            pSession->SendHTTP( "<font color = \"red\">" );
         }
         else if ( Log::TypeWarning == pMessage->type )
         {
            pSession->SendHTTP( "<font color = \"blue\">" );
         }
         else
         {
            pSession->SendHTTP( "<font color = \"black\">" );
         }

         pSession->SendHTTP( pMessage->message );
         pSession->SendHTTP( "</font><br>" );

         ++index;
      }

      pSession->SendHTTP( "</td></tr>" );
      pSession->SendHTTP( "<tr><td class =\"pb\" align = \"center\" colspan = 5><a href = \"doClearLogs?<sessionId>\">Clear Logs</a></td></tr>" );
      pSession->SendHTTP( "</table>" );

      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendShutdownPage(
   Session *pSession
)
{
   char text[ 256 ] = { 0 };

   _snprintf( text, sizeof(text) - 1, "<br><table><tr><td class =\"pb\" align = \"center\">If you are sure you want to <b>shutdown %s</b> please click the link below:<br><br>"
                                      "<a href = \"doShutdown?<sessionId>\">Shutdown %s</a></td></tr></table>", OptionsControl::AppName, OptionsControl::AppName );
   
   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( text );
      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendConfirmClearPicturesPage(
   Session *pSession
)
{

   pSession->BeginSendHTTP( );

      SendHeader( pSession );

      const char *pDay[] = 
      {
         "Sunday",
         "Monday",
         "Tuesday",
         "Wednesday",
         "Thursday",
         "Friday",
         "Saturday",
      };
        
      const char *pDayIndex = strstr(pSession->RequestedPage( ), "confirmClear.") + sizeof("confirmClear.") - 1;

      sint32 index = atoi( pDayIndex );
      
      index = max( index, 0 );
      index = min( index, 6 );

      char message[ MAX_PATH ] = { 0 };
      
      pSession->SendHTTP( "" );
      pSession->SendHTTP( "<table><tr><td class =\"pb\" colspan = 2 align = center>" );

      _snprintf( message, sizeof(message) - 1, "Are you sure you want to clear %s's pictures?", pDay[ index ] );
      pSession->SendHTTP( message );

      pSession->SendHTTP( "</td></tr><tr><td class =\"pb\" align = center>" );

      _snprintf( message, sizeof(message) - 1, "<a href = \"clear.%d?<sessionId>\">Yes</a>", index );
      pSession->SendHTTP( message );

      pSession->SendHTTP( "</td><td class =\"pb\" align = center>" );

      pSession->SendHTTP( "<a href = \"home?<sessionId>\">No</a>" );

      pSession->SendHTTP( "</td></tr></table>" );
      pSession->SendHTTP( "" );

      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendAlbumPage(
   Session *pSession
)
{
   char text[ 256 ] = { 0 };
   char prevLink[ 256 ] = { 0 };
   char nextLink[ 256 ] = { 0 };
   char parentPage[ 256 ] = { 0 };

   //we know it has "album." if it made it to this function
   const char *pIndex = pSession->RequestedPage( ) + sizeof("album.") - 1;
   sint32 index = atoi( pIndex );

   //create the html image tag
   _snprintf( text, sizeof(text) - 1, "<img src = \"pic.%s.jpg?<sessionId>\" width = \"%d\" height = \"%d\" border = \"0\" alt = \"\">", pIndex, g_FileExplorer.GetFileWidth( index ), g_FileExplorer.GetFileHeight( index ) );

   //make a link to the previous picture
   //if it exists
   sint32 prevIndex = g_FileExplorer.GetPrevIndex( index );
   if ( prevIndex >= 0 )
   {
      _snprintf( prevLink, sizeof(prevLink) - 1, "<a href = \"album.%d?<sessionId>\">Previous</a>", prevIndex );
   }

   //make a link to the next picture
   //if it exists
   sint32 nextIndex = g_FileExplorer.GetNextIndex( index );
   if ( nextIndex >= 0 )
   {
      _snprintf( nextLink, sizeof(nextLink) - 1, "<a href = \"album.%d?<sessionId>\">Next</a>", nextIndex );
   }

   const int zero = 0;
   const int *pDay = (const int *) pSession->GetUserData( "day" );
   if ( NULL == pDay ) pDay = (int *) zero;

   const int *pPeriod = (const int *) pSession->GetUserData( "period" );
   if ( NULL == pPeriod ) pPeriod = (int *) zero;
   
   _snprintf( parentPage, sizeof(parentPage) - 1, "<a href = \"day.%d.%d?<sessionId>\">View Thumbnails</a>", 
                                PtrToInt(pDay), PtrToInt(pPeriod) );

   pSession->BeginSendHTTP( );

      SendHeader( pSession );

      pSession->SendHTTP( "" );
      pSession->SendHTTP( "<table>" );

      pSession->SendHTTP( "<tr><td class =\"pb\" align = left>" );
      pSession->SendHTTP( parentPage );
      pSession->SendHTTP( "</td></tr>" );

      pSession->SendHTTP( "<tr><td class =\"pb\" align = left>" );
      pSession->SendHTTP( prevLink );

      if ( prevIndex >= 0 && nextIndex >= 0 )
      {
         pSession->SendHTTP( " | " );
      }
      
      pSession->SendHTTP( nextLink );
      pSession->SendHTTP( "</td></tr>" );

      pSession->SendHTTP( "<tr><td class =\"pb\">" );
      pSession->SendHTTP( text );
      pSession->SendHTTP( "</td></tr>" );

      pSession->SendHTTP( "<tr><td class =\"pb\" align = left>" );
      pSession->SendHTTP( prevLink );
      pSession->SendHTTP( " | " );
      pSession->SendHTTP( nextLink );
      pSession->SendHTTP( "</td></tr>" );
      
      pSession->SendHTTP( "</table>" );
      pSession->SendHTTP( "" );

      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendLoginPage(
   Session *pSession
)
{
   const char login[] =
      "<form method=\"post\" action=\"doLogin?<sessionId>\">"
      "<table>"
      "<tr><td class =\"pb\" align=\"left\">Login:</td><td class =\"pb\" align=\"left\"><input type = \"text\" name = \"login\" value = \"\" size = 20></td></tr>"
      "<tr><td class =\"pb\" align=\"left\">Password:</td><td class =\"pb\" align=\"left\"><input type = \"password\" name = \"password\" value= \"\" size = 20></td></tr>"
      "<tr><td class =\"pb\" align=\"left\" colspan = 2><input type = \"submit\" name = \"submit\" value = \"Submit\"></td></tr>"
      "<tr><td class =\"pb\" align=\"left\" colspan = 2><a href = \"doEmailLostPassword?<sessionId>\">Email lost password</a></td></tr>"
      "<tr><td class =\"pb\" align=\"left\" colspan = 2></td></tr>"
      "<tr><td class =\"pb\" align=\"left\" colspan = 2>If you need assistance please visit our <a href = \"%s\">support page</a>.</td></tr>"
      "</table>"
      "</form>";

   pSession->BeginSendHTTP( );

      SendHeader( pSession );

      char text[ 1024 ] = { 0 };
      _snprintf( text, sizeof(text) - 1, login, OptionsControl::SupportPage );
      pSession->SendHTTP( text );

      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendHomePage(
   Session *pSession
)
{
   pSession->BeginSendHTTP( );

      SendHeader( pSession );

      if ( 0 != GetDaysToExpire( ) ) 
      {  
         SendResource( "home.html", pSession ); 
      }
      else
      {
         char text[ 256 ] = { 0 };

         _snprintf( text, sizeof(text) - 1, "<table><tr><td class =\"pb\" align = \"center\">%s has expired, please <a href = \"%s\">register</a> to restore functionality</td></tr></table>", 
                                             OptionsControl::AppName, OptionsControl::RegisterPage );
         pSession->SendHTTP( text );
      }

      
      if ( NULL == pSession->GetUserData("checkUpdate") )
      {
         pSession->SetUserData( "checkUpdate", IntToPtr(1) );

         // Set up the sockaddr structure
         hostent *pHost = gethostbyname( COMPANY_HOME );

         if ( pHost )
         {
            m_pUpdateString[ 0 ] = NULL;

            char *pIp = inet_ntoa (*(struct in_addr *)*pHost->h_addr_list);

            SOCKADDR_IN service;
            service.sin_family = AF_INET;
            service.sin_addr.s_addr = inet_addr(pIp);
            service.sin_port = htons(80);

            SOCKET webSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

            int result = connect( webSocket, (sockaddr *) &service, sizeof(service) );
            
            const char request [] = "GET /secondlookversion.txt HTTP/1.1\r\n"
                                    "Host: "COMPANY_HOME"\r\n"
                                    "\r\n";


            int length = sizeof(request);
         
            result = send( webSocket, request, length, 0 );
            
            char data[ 1024 ] = { 0 };
            recv( webSocket, data, sizeof(data) - 1, 0 );

            shutdown( webSocket, SD_BOTH );
            closesocket( webSocket );

            char *pStart = strstr( data, "\r\n\r\n" );
            if ( pStart )
            {
               float version = (float) atof( pStart );

               if ( version != X9_VERSION )
               {
                  pStart = strchr( pStart, '-' ) ;
                  if ( pStart )
                  {
                     _snprintf( m_pUpdateString, 1024 - 1, "<br><br><table><tr><td class =\"pb\" align = \"center\">%s", pStart + 1 );
                     m_pUpdateString[ 1024 - 1 ] = NULL;
                  }
               }
            }
         }
      }

      pSession->SendHTTP( m_pUpdateString );

      SendFooter( pSession );
   
   pSession->EndSendHTTP( );
}

void Webserver::SendDayPage(
   Session *pSession
)
{
   const char *pDay = strstr( pSession->RequestedPage( ), "day." ) + sizeof( "day." ) - 1;
   sint32 day = atoi( pDay );
   pDay++;

   sint32 period = 0;

   if ( NULL != *pDay && NULL != *(pDay + 1) ) 
   {
      period = atoi( pDay + 1 );
   }

   period = min( period, 3 );
   period = max( period, 0 );
   
   day = min( day, 6 );
   day = max( day, 0 );

   pSession->SetUserData( "period", IntToPtr(period) );
   pSession->SetUserData( "day", IntToPtr(day) );
   
   char *pStringDay[] = 
   {
      "Sunday",
      "Monday",
      "Tuesday",
      "Wednesday",
      "Thursday",
      "Friday",
      "Saturday",
   };

   char *pStringPeriod[] = 
   {
      "12am to 6am",
      "6am to 12pm",
      "12pm to 6pm",
      "6pm to 12am",
   };

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      
      pSession->SendHTTP( "<a name = \"top\"></a>" );
      pSession->SendHTTP( "" );
      pSession->SendHTTP( "<table cellspacing = 10>" );

      char page[ 256 ] = { 0 };
      _snprintf( page, sizeof(page) - 1, "<tr><td class =\"pb\" align = \"left\" colspan = 6>%s %s</td></tr>", 
                                         pStringDay[ day ], pStringPeriod[ period ] );

      pSession->SendHTTP( page );

      _snprintf( page, sizeof(page) - 1, "<tr><td class =\"pb\" align = \"left\" colspan = 6><a href = \"%s?<sessionId>\">Refresh</a></td></tr>", 
                                         pSession->RequestedPage( ) );

      
      pSession->SendHTTP( page );
      pSession->SendHTTP( "<tr>" );

      sint32 index = g_FileExplorer.GetFirstFileIndex( );
      sint32 size  = g_FileExplorer.GetNumFiles( );
      
      char image[ MAX_PATH ] = { 0 };

      int count = 0;
      int usedCount = 0;

      while ( size )
      {
         char imageName[ MAX_PATH ] = { 0 };
         g_FileExplorer.GetFileName( index, imageName );
         const char *pDash = imageName;
         
         if ( pDash )
         {
            //format is DayOfWeek-MM-DD-YY-HH-MM-SS-MS

            //first get the day of week number (0 - 6)
            //and see if it matches the day the user requested
            int pictureDay = atoi( pDash );

            if ( pictureDay == day )
            {
               int dashCount = 0;
               
               //now forward through the string until we
               //get to the hour
               while ( pDash && *pDash && dashCount < 4 )
               {
                  pDash = strchr( pDash + 1, '-' );
                  ++dashCount;
               }
               
               if ( pDash )
               {
                  //now we have the hour (0 - 24) divide by 6
                  //because our links are broken up into 4 time segments
                  //w 6 hours in each time
                  int picturePeriod = atoi( pDash + 1 ) / 6;
               
                  if ( picturePeriod == period )
                  {
                     //every 6 pictures do a new line
                     if ( usedCount && usedCount % 6 == 0 )
                     {
                        pSession->SendHTTP( "</tr><tr>" );
                     }

                     float aspectRatio = g_FileExplorer.GetFileAspectRatio( index );
                     uint32 height = (uint32) (DEF_THUMB_WIDTH / aspectRatio);

                     if ( g_Options.m_Options.showThumbnails )
                     {
                        _snprintf( image, sizeof(image) - 1, "<td class =\"pb\" align = center><a href = \"album.%d?<sessionId>\"><img src = \"thumb.%d.jpg?<sessionId>\" width = \"%d\" height = \"%d\" border = \"0\" alt = \"\"></a><br>", index, index, DEF_THUMB_WIDTH, height );
                        pSession->SendHTTP( image );
                     }
                     else
                     {
                        _snprintf( image, sizeof(image) - 1, "<td class =\"pb\" align = center><a href = \"album.%d?<sessionId>\"><img src = \"icon.gif?<sessionId>\" border = \"0\" alt = \"\"></a><br>", index, index, DEF_THUMB_WIDTH, height );
                        pSession->SendHTTP( image );
                     }
                     
                     const char *pHour, *pMinute, *pSecond;
                     
                     pHour = pDash + 1;
                     
                     pMinute = strchr( pHour, '-' );
                     if ( pMinute ) pMinute += 1;
                     else pMinute = "";

                     pSecond = strchr( pMinute, '-' );
                     if ( pSecond ) pSecond += 1;
                     else pSecond = "";

                     _snprintf( image, sizeof(image) - 1, "%02d:%02d:%02d</td>", atoi(pHour), atoi(pMinute), atoi(pSecond) );
                     pSession->SendHTTP( image );
                     
                     ++usedCount;
                  }
               }
            }
         }

         index = g_FileExplorer.GetNextIndex( index );

         --size;
      
         ++count;
      };

      //every 6 pictures do a new line
      while ( usedCount % 6 != 0 )
      {
         pSession->SendHTTP( "<td class =\"pb\"></td>" );
         ++usedCount;
      }

      pSession->SendHTTP( "</tr>" );

      {
         pSession->SendHTTP( "<tr>" );

         //send previous page
         int previousDay, previousPeriod;

         if ( 0 == period )
         {
            previousDay = day > 0 ? day - 1 : 6;
            previousPeriod = 3;
         }
         else
         {
            previousDay = day;
            previousPeriod = period - 1;
         }

         _snprintf( page, sizeof(page) - 1, "<td class =\"pb\" align = \"left\"><a href = \"day.%d.%d?<sessionId>\">Previous</a></td>", previousDay, previousPeriod );
         pSession->SendHTTP( page );
   
         pSession->SendHTTP( "<td class =\"pb\"></td><td class =\"pb\"></td><td class =\"pb\"></td><td class =\"pb\"></td>" );

         //send next period
         int nextDay, nextPeriod;

         if ( 3 == period )
         {
            nextDay = 6 == day ? day = 0 : day + 1;
            nextPeriod = 0;
         }
         else
         {
            nextDay = day;
            nextPeriod = period + 1;
         }

         _snprintf( page, sizeof(page) - 1, "<td class =\"pb\" align = \"right\"><a href = \"day.%d.%d?<sessionId>\">Next</a></td>", nextDay, nextPeriod );
         pSession->SendHTTP( page );
      }

      pSession->SendHTTP( "</table>" );
      pSession->SendHTTP( "<a href=\"#top\">Back To Top</a>" );
      pSession->SendHTTP( "" );

      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::SendThumbnail(
   Session *pSession
)
{
   pSession->BeginSendHTTP( "image/jpg" );

      const char *pImageIndex = strstr(pSession->RequestedPage( ), "thumb.") + sizeof("thumb.") - 1;

      sint32 index = atoi(pImageIndex);

      char *pBuffer;
      uint32 fileSize;
      g_FileExplorer.GetThumbnail( index, (void **) &pBuffer, &fileSize );

      pSession->SendHTTP( pBuffer, fileSize );

      delete [] pBuffer;

   pSession->EndSendHTTP( );
}

void Webserver::SendImage(
   Session *pSession
)
{
   pSession->BeginSendHTTP( "image/jpg" );

      const char *pImageIndex = strstr(pSession->RequestedPage( ), "pic.") + sizeof("pic.") - 1;

      sint32 index = atoi(pImageIndex);

      char image[ MAX_PATH ] = { 0 };

      char *pBuffer;
      sint32 fileSize;

      g_FileExplorer.GetFile( index, (void **) &pBuffer );
      fileSize = g_FileExplorer.GetFileSize( index );

      pSession->SendHTTP( pBuffer, fileSize );

      delete [] pBuffer;

   pSession->EndSendHTTP( );
}

void Webserver::DoLogin(
   Session *pSession
)
{
   bool success = false;

   do
   {
      const char *pLogin = pSession->GetPostData( "login" );
      const char *pPassword  = pSession->GetPostData( "password" );
      
      if ( NULL == pLogin ) break;
      if ( NULL == pPassword ) break;

      if ( 0 != strcmp(pLogin, g_Options.m_Options.login) ) break;
      if ( 0 != strcmp(pPassword, g_Options.m_Options.password) ) break;

      success = true;

   } while ( 0 );

   if ( true == success )
   {
      pSession->LogIn( );
   }
}

void Webserver::DoEnterKey(
   Session *pSession
)
{
   char text[ 512 ] = { 0 };

   const char *pKey = pSession->GetPostData( "key" );
   if ( NULL == pKey ) pKey = "";
   
   ProductKey::Key key = { 0 };

   pSession->BeginSendHTTP( );
   
   if ( strlen(pKey) == sizeof(key.key) - 1 )
   {
      int seed;

      strncpy( key.key, pKey, sizeof(key.key) - 1 ); 
      
      if ( TRUE == ProductKey::VerifyKey(&key, &seed) )
      {
         memcpy( &g_Options.m_Options.productKey, &key, sizeof(ProductKey::Key) );
         g_Options.m_Options.installTime.dwHighDateTime = INT_MAX;
         g_Options.m_Options.installTime.dwLowDateTime  = INT_MAX;
         g_Options.m_IsDirty = TRUE;
         
         SendHeader( pSession );

         _snprintf( text, sizeof(text) - 1, "<table><tr><td class =\"pb\" align = \"center\">Thank you for registering %s!</td></tr>", OptionsControl::AppName );
      }
   }

   //key was not verified
   if ( GetDaysToExpire( ) >= 0 )
   {
      SendHeader( pSession );

      _snprintf( text, sizeof(text) - 1, "<table><tr><td class =\"pb\" align = \"center\">I'm sorry <b>%s</b> is not a valid key.</td></tr>"
                                         "<tr><td class =\"pb\" align = \"center\">If you need further assistance please visit our <a href = \"%s\">support page</a>.</td></tr>", 
                                         pKey, OptionsControl::SupportPage );
   }

   pSession->SendHTTP( text );
   pSession->SendHTTP( "<tr><td class =\"pb\" align = \"center\"><a href = \"home?<sessionId>\">Home</a></td></tr>" );
   pSession->SendHTTP( "</table>" );
   
   SendFooter( pSession );
   pSession->EndSendHTTP( );
}

void Webserver::DoServerOptions(
   Session *pSession
)
{
   const char *pParam;

   char customServerOptions[ 2048 ];

   //customize error and success messages and send page back to the user
   char changeString[ 1024 ] = "<table><tr><td class =\"pb\" align = \"left\"><OL><font color = \"green\"><b>";

   pParam = pSession->GetPostData( "port" );
   
   if ( pParam )
   {
      short port = (short)  atoi( pParam );
   
      if ( port != g_Options.m_Options.port )
      {
         g_Options.m_Options.port = port;
         m_NeedsRestart = TRUE;

         _snprintf( changeString, sizeof(changeString) - 1, "%s<LI>Your port was set to port: %d", changeString, port );
      }
   }

   pParam = pSession->GetPostData( "showInSystray" );

   //if param is there, the box was checked
   if ( pParam )
   {
      //if the prev was not checked, report the change
      if ( 0 == g_Options.m_Options.showInSystray )
      {
         g_Options.m_Options.showInSystray = 1;
         _snprintf( changeString, sizeof(changeString) - 1, "%s<LI>Show in system tray", changeString );
         g_Log.Write( Log::TypeInfo, "Show in system tray" );

         g_CommandBuffer.Write( "systray show" );
      }
   }
   //param wasn't there so it was not checked
   else
   {
      //if the prev was checked, report the change
      if ( g_Options.m_Options.showInSystray )
      {
         g_Options.m_Options.showInSystray = 0;
         _snprintf( changeString, sizeof(changeString) - 1, "%s<LI>Hide from system tray", changeString );
         g_Log.Write( Log::TypeInfo, "Hide from system tray" );

         g_CommandBuffer.Write( "systray hide" );
      }
   }

   pParam = pSession->GetPostData( "allowRemoteAccess" );

   //if param is there, the box was checked
   if ( pParam )
   {
      //if the prev was not checked, report the change
      if ( 0 == g_Options.m_Options.allowRemoteAccess )
      {
         g_Options.m_Options.allowRemoteAccess = 1;
         _snprintf( changeString, sizeof(changeString) - 1, "%s<LI>Enable Remote Access", changeString );
         g_Log.Write( Log::TypeInfo, "Allow Remote Access" );
      }
   }
   //param wasn't there so it was not checked
   else
   {
      //if the prev was checked, report the change
      if ( g_Options.m_Options.allowRemoteAccess )
      {
         g_Options.m_Options.allowRemoteAccess = 0;
         _snprintf( changeString, sizeof(changeString) - 1, "%s<LI>Disable Remote Access", changeString );
         g_Log.Write( Log::TypeInfo, "Disable Remote Access" );
      }
   }

   pParam = pSession->GetPostData( "oldPassword" );
   
   if ( pParam && pParam[ 0 ] )
   {
      //if the old password is correct, allow them to change the password
      if ( 0 == strcmp(pParam, g_Options.m_Options.password) )
      {
         //verify we have a new password and they have confirmed it
         const char *pNewPass = pSession->GetPostData( "newPassword" );
         const char *pConfirmPass = pSession->GetPostData( "confirmPassword" );
            
         if ( pNewPass )
         {
            if ( pConfirmPass )
            {
               //do not allow the new password if it is too big 
               if ( strlen(pNewPass) < sizeof(g_Options.m_Options.password) - 1 )
               {
                  if ( 0 == strcmp(pNewPass, pConfirmPass) )
                  {
                     strncpy( g_Options.m_Options.password, pNewPass, sizeof(g_Options.m_Options.password) - 1 );
                     strncat( changeString, "<LI>Your password was changed", sizeof(changeString) - 1 - strlen(changeString) );
                  }
                  else
                  {
                     strncat( changeString, "</font><font color = \"red\"><LI>Your new password and confirm password did not match, so your password was not changed", sizeof(changeString) - 1 - strlen(changeString) );
                  }
               }
               else
               {
                  _snprintf( changeString, sizeof(changeString) - 1, "</font><font color = \"red\">%s<LI>Your password must be less than %d characters, so your password was not changed", changeString, sizeof(g_Options.m_Options.password) - 1 );
               }
            }
            else
            {
               strncat( changeString, "</font><font color = \"red\"><LI>Please enter a confirm password (to match the new password) if you would like your password changed", sizeof(changeString) - 1 - strlen(changeString) );
            }
         }
         else
         {
            strncat( changeString, "</font><font color = \"red\"><LI>Please enter a new password if you would like your password changed", sizeof(changeString) - 1 - strlen(changeString) );
         }
      }
      else
      {
         strncat( changeString, "</font><font color = \"red\"><LI>Your old password was not valid, so your password was not changed", sizeof(changeString) - 1 - strlen(changeString) );
      }
   }

   strncat( changeString, "</b></font></OL></td></tr></table>", sizeof(changeString) - 1 - strlen(changeString) );

   BuildServerOptionsPage( customServerOptions, sizeof(customServerOptions) );

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( changeString );
      pSession->SendHTTP( customServerOptions );
      SendFooter( pSession );

   pSession->EndSendHTTP( );

   g_Options.m_IsDirty = TRUE;
}

void Webserver::DoConfigureEmail(
   Session *pSession
)
{
   const char *pParam;

   char customEmail[ 2048 ];

   //customize error and success messages and send page back to the user
   char changeString[ 1024 ] = "<table><tr><td class =\"pb\" align = \"left\"><font color = \"green\">Your email settings have been updated.";

   pParam = pSession->GetPostData( "status" );
   
   if ( pParam )
   {
      if ( 0 == strcmp(pParam, "on") )
      {
         g_Options.m_Options.email.on = 1;

         //start emailing pictures from this point on
         g_Options.m_Options.email.lastFileId = g_FileExplorer.GetNextFileUniqueID( );
      }
      else
      {
         g_Options.m_Options.email.on = 0;
      }
   }

   pParam = pSession->GetPostData( "email" );
   
   if ( pParam )
   {
      ParseHTTPFormat( g_Options.m_Options.email.address, pParam, sizeof(g_Options.m_Options.email.address) - 1 );
   }

   pParam = pSession->GetPostData( "emailPicsPerHour" );
   
   if ( pParam )
   {
      int value = atoi( pParam );
      if ( value > 10 ) value = 10;
      if ( value < 1 )  value = 1;

      g_Options.m_Options.email.picsPerHour = value;
   }

   pParam = pSession->GetPostData( "emailPassword" );
   
   if ( pParam )
   {
      ParseHTTPFormat( g_Options.m_Options.email.password, pParam, sizeof(g_Options.m_Options.email.password) - 1 );
   }

   strncat( changeString, "</font></td></tr><tr><td class =\"pb\">", sizeof(changeString) - 1 - strlen(changeString) );

   m_SmtpSender.Stop( );

   if ( g_Options.m_Options.email.on )
   {
      m_SmtpSender.SendFrom( g_Options.m_Options.email.address );
      m_SmtpSender.SendTo( g_Options.m_Options.email.address );
      m_SmtpSender.Password( g_Options.m_Options.email.password );
      m_SmtpSender.Subject( "%s - Email Validation Test", OptionsControl::AppName );
      m_SmtpSender.Message( "Receiving this email verifies your %s email settings are correct.", OptionsControl::AppName );

      m_SmtpSender.Send( );
   }

   strncat( changeString, "</td></tr></table>", sizeof(changeString) - 1 - strlen(changeString) );
   
   if ( g_Options.m_Options.email.on )
   {
      strncat( changeString, "<table><tr><td class =\"pb\" align = \"center\"><font color = \"green\">An email has been sent to verify your settings.  "
                             "If you do not receive the email shortly please check the Diagnostics page.</font></td></tr></table>", 
                             sizeof(changeString) - 1 - strlen(changeString) );
   }

   BuildConfigureEmailPage( customEmail, sizeof(customEmail) );

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( changeString );
      pSession->SendHTTP( customEmail );
      SendFooter( pSession );

   pSession->EndSendHTTP( );

   g_Options.m_IsDirty = TRUE;
}

void Webserver::DoEmailLostPassword(
   Session *pSession   
)
{
   m_SmtpSender.Stop( );

   m_SmtpSender.SendFrom( g_Options.m_Options.email.address );
   m_SmtpSender.SendTo( g_Options.m_Options.email.address );
   m_SmtpSender.Password( g_Options.m_Options.email.password );
   m_SmtpSender.Subject( "%s - Lost Password", OptionsControl::AppName );
   m_SmtpSender.Message( "Your password is: %s.", g_Options.m_Options.password );

   m_SmtpSender.Send( );
   
   const char text[] =
      "<table><tr><td class =\"pb\" align = \"center\">If your email was configured correctly, your password has been emailed to your account.<br><br>"
      "<a href = \"home?<sessionId>\">Home</a></td></tr></table>";

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( text );
      SendFooter( pSession );

   pSession->EndSendHTTP( );
}

void Webserver::DoShutdown(
   Session *pSession   
)
{
   char text[ 128 ] = { 0 };

   _snprintf( text, sizeof(text) - 1, "<table><tr><td class =\"pb\" align = \"center\">%s will be shutdown immediately</td></tr></table>", OptionsControl::AppName );

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( text );
      SendFooter( pSession );

   pSession->EndSendHTTP( );

   g_CommandBuffer.Write( "core shutdown" );
}

void Webserver::DoConfigurePictures(
   Session *pSession
)
{
   uint32 dayIndex, periodIndex;
   const char *pValue;

   const char *pDays[] = 
   {
      "sunday",
      "monday",
      "tuesday",
      "wednesday",
      "thursday",
      "friday",
      "saturday",
   };

   // now update settings
   memset( &g_Options.m_Options.days, 0, sizeof(g_Options.m_Options.days) );

   for ( dayIndex = 0; dayIndex < 7; dayIndex++ )
   {  
      periodIndex = 0;

      while ( NULL != (pValue = pSession->GetPostData(pDays[ dayIndex ], periodIndex)) )
      {
         uint32 period = atoi( pValue );
         period = min( period, 3 );
         
         g_Options.m_Options.days[ dayIndex ].periods[ period ] = 1;

         ++periodIndex;
      }
   }
   
   pValue = pSession->GetPostData( "screenCapSize" );
   if ( NULL != pValue )
   {
      if ( 0 == strcmp(pValue, "full") )
      {
         g_Options.m_Options.screenCapSize = ScreenCapSize_Full;
      }
      else if ( 0 == strcmp(pValue, "medium") )
      {
         g_Options.m_Options.screenCapSize = ScreenCapSize_Medium;
      }
      else if ( 0 == strcmp(pValue, "small") )
      {
         g_Options.m_Options.screenCapSize = ScreenCapSize_Small;
      }
   }

   pValue = pSession->GetPostData( "showThumbnails" );
   if ( NULL != pValue )
   {
      if ( 0 == strcmp(pValue, "true") )
      {
         g_Options.m_Options.showThumbnails = 1;
      }
      else
      {
         g_Options.m_Options.showThumbnails = 0;
      }      
   }
   
   pValue = pSession->GetPostData( "picsPerHour" );
   if ( NULL != pValue )
   {
      g_Options.m_Options.picsPerHour = min( MAX_PICS_PER_HOUR, (unsigned char) atoi( pValue ) );
   }

   pValue = pSession->GetPostData( "daysToKeep" );
   if ( NULL != pValue )
   {
      unsigned short days = (unsigned short) atoi( pValue );

      days = min( days, 7 );
      g_Options.m_Options.daysToKeep = max( days, 1 );
   }

   char *pPictures = new char[ 1024 * 10 ];
   BuildConfigurePicturesPage( pPictures, 1024 * 10 );

   pSession->BeginSendHTTP( );

      SendHeader( pSession );
      pSession->SendHTTP( "<table><tr><td class =\"pb\" align = \"center\"><font color = \"green\"><b>Your pictures have been configured</b></font></td></tr></table>" );
      pSession->SendHTTP( pPictures );
      SendFooter( pSession );

   pSession->EndSendHTTP( );

   delete [] pPictures;

   g_Options.m_IsDirty = TRUE;
}

void Webserver::DoClearPictures(
   Session *pSession
)
{
   const char *pIndex = pSession->RequestedPage( ) + sizeof("clear.") - 1;

   sint32 dayIndex = atoi( pIndex );

   dayIndex = max( dayIndex, 0 );
   dayIndex = min( dayIndex, 6 );

   bool pictureFound = false;

   int totalPicsToDelete = 0;

   //continue to query index and size
   //in a do/while loop because deleting a picture
   //will change the index and size
   sint32 index = g_FileExplorer.GetFirstFileIndex( );
   sint32 size  = g_FileExplorer.GetNumFiles( );

   char image[ MAX_PATH ] = { 0 };

   while ( size )
   {
      char imageName[ MAX_PATH ] = { 0 };
      g_FileExplorer.GetFileName( index, imageName );
      const char *pDash = imageName;
      
      if ( pDash )
      {
         //format is DayOfWeek-MM-DD-YY-HH-MM-SS-MS

         //first get the day of week number (0 - 6)
         //and see if it matches the day the user requested
         int pictureDay = atoi( pDash );

         if ( pictureDay == dayIndex )
         {
            ++totalPicsToDelete;
         }
      }

      index = g_FileExplorer.GetNextIndex( index );

      --size;
   }

   Clock clock;
   clock.Start( );

   float *pProgressPercent = g_FileExplorer.StartProgressBarDisplay( "Please wait while your pictures are deleted." );

   sint32 picsDeleted = 0;

   do
   {
      pictureFound = false;

      //continue to query index and size
      //in a do/while loop because deleting a picture
      //will change the index and size
      sint32 index = g_FileExplorer.GetFirstFileIndex( );
      sint32 size  = g_FileExplorer.GetNumFiles( );
      
      char image[ MAX_PATH ] = { 0 };

      while ( size )
      {
         char imageName[ MAX_PATH ] = { 0 };
         g_FileExplorer.GetFileName( index, imageName );
         const char *pDash = imageName;
         
         if ( pDash )
         {
            //format is DayOfWeek-MM-DD-YY-HH-MM-SS-MS

            //first get the day of week number (0 - 6)
            //and see if it matches the day the user requested
            int pictureDay = atoi( pDash );

            if ( pictureDay == dayIndex )
            {
               
               //Clock deleteClock;
               //deleteClock.Start( );

               //_RPT0( _CRT_WARN, "-----\n" );

               g_FileExplorer.DeleteFile( index );

               //_RPT1( _CRT_WARN, "Delete pic: %.4f\n", deleteClock.MarkSample( ) );
               
               picsDeleted++;
               
               //restart our loop because
               //deleting an image changes
               //our index and num files
               pictureFound = true;
               break;
            }
         }

         index = g_FileExplorer.GetNextIndex( index );

         --size;
      }

      *pProgressPercent = (float) picsDeleted / totalPicsToDelete;
   }
   while ( pictureFound );

   _RPT2( _CRT_WARN, "Deleted %d pics: %.4f\n", totalPicsToDelete, clock.MarkSample( ) );

   g_FileExplorer.StopProgressBarDisplay( );

   SendHomePage( pSession );
}

void Webserver::MakeHttpFormat(
   char *pHttpString,
   const char *pString,
   uint32 httpStringSize
)
{
   size_t i, length = strlen(pString);

   char *pDest = pHttpString;
   
   length = min( length, httpStringSize - 1 );

   for ( i = 0; i < length; i++ )
   {
      if ( ' ' == pString[ i ] )
      {
         *pDest++ = '%';
         *pDest++ = '2';
         *pDest++ = '0';
      }
      else if ( '(' == pString[ i ] )
      {
         *pDest++ = '%';
         *pDest++ = '2';
         *pDest++ = '8';
      }
      else if ( ')' == pString[ i ] )
      {
         *pDest++ = '%';
         *pDest++ = '2';
         *pDest++ = '9';
      }
      else
      {
         *pDest++ = pString[ i ];
      }
   }

   pDest[ i ] = NULL;
}

void Webserver::ParseHTTPFormat(
   char *pString,
   const char *pHttpString,
   uint32 stringSize
)
{
   size_t i, length = strlen(pHttpString);
   
   int destIndex = 0;

   for ( i = 0; i < length; i++ )
   {
      if ( '%' == pHttpString[ i + 0 ] &&
           '2' == pHttpString[ i + 1 ] &&
           '0' == pHttpString[ i + 2 ] )
      {
         pString[ destIndex++ ] = ' ';
         i += 2;
      }
      else if ( '%' == pHttpString[ i + 0 ] &&
                '2' == pHttpString[ i + 1 ] &&
                '8' == pHttpString[ i + 2 ] )
      {
         pString[ destIndex++ ] = '(';
         i += 2;
      }
      else if ( '%' == pHttpString[ i + 0 ] &&
                '2' == pHttpString[ i + 1 ] &&
                '9' == pHttpString[ i + 2 ] )
      {
         pString[ destIndex++ ] = ')';
         i += 2;
      }
      else if ( '%' == pHttpString[ i + 0 ] &&
                '4' == pHttpString[ i + 1 ] &&
                '0' == pHttpString[ i + 2 ] )
      {
         pString[ destIndex++ ] = '@';
         i += 2;
      }
      else
      {
         pString[ destIndex++ ] = pHttpString[ i ];         
      }

      if ( destIndex == stringSize ) break;
   }

   pString[ destIndex  ] = NULL;
}

void Webserver::BuildServerOptionsPage(
   char *pString,
   size_t size
)
{
   const char serverOptions[] =
      "<form method=\"post\" action=\"doServerOptions?<sessionId>\">"
      "<table>"
      "<tr><td class =\"pb\" align=\"left\" colspan = 2><font color = \"red\">Do not change <b>port</b> unless you know what you are doing.</font></td></tr>"
      "<tr><td class =\"pb\" align=\"left\" colspan = 2><font color = \"red\">This will cause the webserver to restart on the new port.</font></td></tr>"
      "<tr><td class =\"pb\" align=\"left\">Port:</td><td class =\"pb\" align=\"left\"><input type = \"text\" name = \"port\" value = \"%d\" size = 5></td></tr>"
      "<tr><td class =\"pb\" align=\"left\">Allow Remote Access:</td><td class =\"pb\" align=\"left\"><input type = \"checkbox\" name = \"allowRemoteAccess\" %s value = \"true\"></td></tr>"
      "<tr><td class =\"pb\" align=\"left\">Old Password:</td><td class =\"pb\" align=\"left\"><input type=\"password\" name=\"oldPassword\" value=\"\"></td></tr>"
      "<tr><td class =\"pb\" align=\"left\">New Password:</td><td class =\"pb\" align=\"left\"><input type=\"password\" name=\"newPassword\" value=\"\"  maxlength = \"%d\"></td></tr>"
      "<tr><td class =\"pb\" align=\"left\">Confirm New Password:</td><td class =\"pb\" align=\"left\"><input type=\"password\" name=\"confirmPassword\" value=\"\" maxlength = \"%d\"></td></tr>"
      "<tr><td class =\"pb\" align=\"left\">Show icon in system tray:</td><td class =\"pb\" align=\"left\"><input type=\"checkbox\" name=\"showInSystray\" %s value = \"true\"></td></tr>"
      "<tr><td class =\"pb\" align=\"left\" colspan = 2><input type = \"submit\" name = \"submit\" value = \"Submit\"></td></tr>"
      "</table>"
      "</form>";

   _snprintf( pString, size - 1, serverOptions, g_Options.m_Options.port, g_Options.m_Options.allowRemoteAccess ? "checked" : "", sizeof(g_Options.m_Options.password) - 1, 
                                                sizeof(g_Options.m_Options.password) - 1, g_Options.m_Options.showInSystray ? "checked" : "" );
   pString[ size - 1 ] = NULL;
}

void Webserver::BuildConfigureEmailPage(
   char *pString,
   size_t size
)
{
   const char emailOptions[] =
      "<table><tr><td class =\"pb\" align=\"left\">"
      "<OL>"
      "<LI>Because your security is extremely important to us, we only send to secure email servers"
      "<LI>Currently the only secure email server we support is: <a href=\"http://www.gmail.com\">gmail</a>"
      "<LI>In order to establish a secure connection with the email server, we require your email login and password"
      "<LI>This information will be sent encrypted to the SMTP server to allow our secure screen capture emails"
      "</OL>"
      "</td></tr></table>"
      "<br>"
      "<form method=\"post\" action=\"doConfigureEmail?<sessionId>\">"
      "<table>"
      "<tr><td class =\"pb\" align=\"left\">Send Hourly Pictures:</td><td class =\"pb\" align=\"left\"><input type = \"radio\" name = \"status\" value = \"on\" %s>On <input type = \"radio\" name = \"status\" value = \"off\" %s>Off</td></tr>"
      "<tr><td class =\"pb\" align=\"left\">Email:</td><td class =\"pb\" align=\"left\"><input type = \"text\" name = \"email\" value = \"%s\" size = \"48\" maxlength = \"%d\"></td></tr>"
      "<tr><td class =\"pb\" align=\"left\">Password:</td><td class =\"pb\" align=\"left\"><input type = \"password\" name = \"emailPassword\" value = \"%s\" size = \"48\" maxlength = \"%d\"></td></tr>"
      "<tr><td class =\"pb\" align=\"left\">Pictures per hour (1 - 10):</td><td class =\"pb\" align=\"left\"><input type=\"text\" name=\"emailPicsPerHour\" value=\"%d\" size = 3 maxlength = \"2\"></td></tr>"
      "<tr><td class =\"pb\" align=\"left\" colspan = 2><input type = \"submit\" name = \"submit\" value = \"Submit\"></td></tr>"
      "</table>"
      "</form>";

   _snprintf( pString, size - 1, emailOptions, g_Options.m_Options.email.on ? "checked" : "", g_Options.m_Options.email.on ? "" : "checked", 
                                               g_Options.m_Options.email.address,  sizeof(g_Options.m_Options.email.address) - 1, 
                                               g_Options.m_Options.email.password, sizeof(g_Options.m_Options.email.password) - 1, 
                                               (int) g_Options.m_Options.email.picsPerHour );
   pString[ size - 1 ] = NULL;
}

void Webserver::BuildConfigurePicturesPage(
   char *pString,
   size_t size
)
{
   const char config[] =
      "<form method=\"post\" action=\"doConfigurePictures?<sessionId>\">"
      ""
      "<table>"
      "<tr><td class =\"pb\" align = center><b>Sunday</b></td><td class =\"pb\" align = center><b>Monday</b></td><td class =\"pb\" align = center><b>Tuesday</b></td><td class =\"pb\" align = center><b>Wednesday</b></td>"
         "</tr>"
         "<tr>"                                                             //add spaces here which will affect all cells in the table
         "<td class =\"pb\" align = left><input type = checkbox name = sunday %s value = 0>12am to 6am&nbsp&nbsp&nbsp&nbsp</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = monday %s value = 0>12am to 6am&nbsp&nbsp&nbsp&nbsp</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = tuesday %s value = 0>12am to 6am&nbsp&nbsp&nbsp&nbsp</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = wednesday %s value = 0>12am to 6am&nbsp&nbsp&nbsp&nbsp</td>"
         "</tr>"
         "<tr>"
         "<td class =\"pb\" align = left><input type = checkbox name = sunday %s value = 1>6am to 12pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = monday %s value = 1>6am to 12pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = tuesday %s value = 1>6am to 12pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = wednesday %s value = 1>6am to 12pm</td>"
         "</tr>"
         "<tr>"
         "<td class =\"pb\" align = left><input type = checkbox name = sunday %s value = 2>12pm to 6pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = monday %s value = 2>12pm to 6pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = tuesday %s value = 2>12pm to 6pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = wednesday %s value = 2>12pm to 6pm</td>"
         "</tr>"
         "<tr>"
         "<td class =\"pb\" align = left><input type = checkbox name = sunday %s value = 3>6pm to 12am</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = monday %s value = 3>6pm to 12am</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = tuesday %s value = 3>6pm to 12am</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = wednesday %s value = 3>6pm to 12am</td>"
         "</tr>"
      "</table>"
      "<br>"
      "<table>"
      "<tr><td class =\"pb\" align = center><b>Thursday</b></td><td class =\"pb\" align = center><b>Friday</b></td><td class =\"pb\" align = center><b>Saturday</b></td>"
         "</tr>"
         "<tr>"                                                             //add spaces here which will affect all cells in the table
         "<td class =\"pb\" align = left><input type = checkbox name = thursday %s value = 0>12am to 6am&nbsp&nbsp&nbsp&nbsp</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = friday %s value = 0>12am to 6am&nbsp&nbsp&nbsp&nbsp</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = saturday %s value = 0>12am to 6am&nbsp&nbsp&nbsp&nbsp</td>"
         "</tr>"
         "<tr>"
         "<td class =\"pb\" align = left><input type = checkbox name = thursday %s value = 1>6am to 12pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = friday %s value = 1>6am to 12pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = saturday %s value = 1>6am to 12pm</td>"
         "</tr>"
         "<tr>"
         "<td class =\"pb\" align = left><input type = checkbox name = thursday %s value = 2>12pm to 6pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = friday %s value = 2>12pm to 6pm</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = saturday %s value = 2>12pm to 6pm</td>"
         "</tr>"
         "<tr>"
         "<td class =\"pb\" align = left><input type = checkbox name = thursday %s value = 3>6pm to 12am</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = friday %s value = 3>6pm to 12am</td>"
         "<td class =\"pb\" align = left><input type = checkbox name = saturday %s value = 3>6pm to 12am</td>"
         "</tr>"
      "</table>"
      "<br><hr>"
      ""
      "<table>"
         "<tr><td class =\"pb\" align=\"left\">Pictures to take each hour (0 - %d):</td><td class =\"pb\" align=\"left\"><input type = \"text\" name = \"picsPerHour\" value = \"%d\" maxlength = 2 size = 3 ></td></tr>"
         "<tr><td class =\"pb\" align=\"left\">Keep pictures at least this many days (1 - 7):</td><td class =\"pb\" align=\"left\"><input type = \"text\" name = \"daysToKeep\" value = \"%d\" maxlength = 3 size = 3 ></td></tr>"
         "<tr><td class =\"pb\" align=\"left\">Show Thumbnails:</td><td class =\"pb\" align=\"left\"><select name=showThumbnails><option value=true %s>Yes</option><option value=false %s>No</option></select></td></tr>"
         "<tr><td class =\"pb\" align=\"left\" colspan = 2><input type = \"submit\" name = \"submit\" value = \"Submit\"></td></tr>"
      "</table>"
      "</form>";

   
   uint32 i, c, totalSize = 0, totalPeriods = 0;

   for ( i = 0; i < 7; i++ )
   {
      for ( c = 0; c < 4; c++ )
      {
         totalPeriods += g_Options.m_Options.days[ i ].periods[ c ];
      }
   }
   
   totalSize = totalPeriods * 6 * g_Options.m_Options.picsPerHour;
   totalSize *= 80; //estimated jpeg for 640x480KB

   const char *pSmall = "", *pMedium = "", *pFull = "";

   if ( ScreenCapSize_Small == g_Options.m_Options.screenCapSize )
   {
      pSmall = "selected";
   }
   else if ( ScreenCapSize_Medium == g_Options.m_Options.screenCapSize )
   {
      pMedium = "selected";
   }
   else if ( ScreenCapSize_Full == g_Options.m_Options.screenCapSize )
   {
      pFull = "selected";
   }
   
   _snprintf( pString, size - 1, config,g_Options.m_Options.days[ 0 ].periods[ 0 ]  ? "checked" : "", 
                                        g_Options.m_Options.days[ 1 ].periods[ 0 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 2 ].periods[ 0 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 3 ].periods[ 0 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 0 ].periods[ 1 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 1 ].periods[ 1 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 2 ].periods[ 1 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 3 ].periods[ 1 ]  ? "checked" : "", 
                                        g_Options.m_Options.days[ 0 ].periods[ 2 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 1 ].periods[ 2 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 2 ].periods[ 2 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 3 ].periods[ 2 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 0 ].periods[ 3 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 1 ].periods[ 3 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 2 ].periods[ 3 ]  ? "checked" : "", 
                                        g_Options.m_Options.days[ 3 ].periods[ 3 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 4 ].periods[ 0 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 5 ].periods[ 0 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 6 ].periods[ 0 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 4 ].periods[ 1 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 5 ].periods[ 1 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 6 ].periods[ 1 ]  ? "checked" : "", 
                                        g_Options.m_Options.days[ 4 ].periods[ 2 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 5 ].periods[ 2 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 6 ].periods[ 2 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 4 ].periods[ 3 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 5 ].periods[ 3 ]  ? "checked" : "",
                                        g_Options.m_Options.days[ 6 ].periods[ 3 ]  ? "checked" : "",
                                        MAX_PICS_PER_HOUR, g_Options.m_Options.picsPerHour, g_Options.m_Options.daysToKeep, 
                                        g_Options.m_Options.showThumbnails ? "selected" : "", g_Options.m_Options.showThumbnails ? "" : "selected");


   pString[ size - 1 ] = NULL;
}

void Webserver::SendHeader(
   Session *pSession
)
{
   if ( pSession->IsSecure( ) )
   {
      SendResource( "secureHeader.html", pSession );
   }
   else
   {
      SendResource( "header.html", pSession );
   }

   pSession->SendHTTP( "" );
   
   int daysToExpire = GetDaysToExpire( );

   if ( daysToExpire >= 0 )
   {
      char header[ 1024 ];

      if ( daysToExpire > 0 )
      {
         const char *pFontTag = "";

         _snprintf( header, sizeof(header) - 1, "<table width = \"800\" bgcolor = \"#afc8fd\"><tr><td class =\"pb\" align = \"center\">%sYou have %d day%s left to <a href = \"%s\">register</a></td></tr>"
                                                "<tr><td class =\"pb\" align = \"center\"><a href = \"enterKey?<sessionId>\">Enter Product Key</a></td></tr>",
                                                pFontTag, daysToExpire, daysToExpire > 1 ? "s" : "", OptionsControl::RegisterPage );

      #ifdef _DEBUG
            _snprintf( header, sizeof(header) - 1, "%s<tr><td class =\"pb\" align = \"center\"><a href = \"generateKey?<sessionId>\">Generate Key</a></td></tr>",
                                                   header );
      #endif
      }
      else if ( 0 == daysToExpire )
      {
         _snprintf( header, sizeof(header) - 1, "<table width = \"800\" bgcolor = \"#afc8fd\"><tr><td class =\"pb\" align = \"center\"> %s has expired, please <a href = \"%s\">register</a></td></tr>"
                                                "<tr><td class =\"pb\" align = \"center\"><a href = \"enterKey?<sessionId>\">Enter Product Key</a></td></tr>",
                                                OptionsControl::AppName, OptionsControl::RegisterPage );

      #ifdef _DEBUG
            _snprintf( header, sizeof(header) - 1, "%s<tr><td class =\"pb\" align = \"center\"><a href = \"generateKey?<sessionId>\">Generate Key</a></td></tr>",
                                                   header );
      #endif
      }

      strncat( header, "</table>", sizeof(header) - strlen(header) - 1 );
          
      pSession->SendHTTP( header );
   }
}

void Webserver::SendFooter(
   Session *pSession
)
{
   SendResource( "footer.html", pSession );
}

void Webserver::SendResource(
   const char *pResourceName,
   Session *pSession
)
{
   ResourceDesc desc;       
   
   if ( m_ResourceHash.Get(pResourceName, &desc) )
   {
      BYTE *pData;
      UINT size;

      pData = (BYTE *)LockResource( desc.hResource );
      size  = SizeofResource( GetModuleHandle( NULL ), desc.hInfo );
      pSession->SendHTTP( pData, size );          
   }
   else
   {
      char fullPath[ MAX_PATH ] = "html\\";
      strncat( fullPath, pResourceName, sizeof(fullPath) - strlen(fullPath) - 1 ); 
      fullPath[ sizeof(fullPath) - 1 ] = NULL;

      FILE *pFile = fopen( fullPath, "rb" );

      if ( NULL != pFile )
      {
         void *pData;
         size_t size;

         fseek( pFile, 0, SEEK_END );
         size = ftell( pFile );

         fseek( pFile, 0, SEEK_SET );
         
         pData = malloc( size );

         fread( pData, size, 1, pFile );
         fclose( pFile );
   
         pSession->SendHTTP( pData, size );          
         free( pData );
      }
   }
}

void Webserver::SendHourlyEmail( void )
{
   if ( g_Options.m_Options.email.on > 0 && g_Options.m_Options.email.picsPerHour )
   {
      m_SmtpSender.Stop( );

      uint32 newPicUniqueIds[ 128 ];

      sint32 index = g_FileExplorer.GetFirstFileIndex( );
      sint32 size  = g_FileExplorer.GetNumFiles( );
      
      uint32 i, newPics = 0;

      //loop through all the files and save the file indices
      //which have an id later than the last picture sent
      while ( size )
      {
         uint32 fileId = g_FileExplorer.GetFileUniqueID( index );
      
         //if it's later than the last picture sent, save the index
         if ( fileId > g_Options.m_Options.email.lastFileId )
         {
            newPicUniqueIds[ newPics++ ] = fileId;
         }

         //prevent a buffer overflow if there are tons of new pictures
         //(should never happen because max pics per hour is 60)
         if ( newPics == sizeof(newPicUniqueIds) / sizeof(uint32) ) break;

         index = g_FileExplorer.GetNextIndex( index );

         --size;
      };

      //if there are new pics, then send a sample of them
      if ( newPics > 0 )
      {
         //prepare the email
         m_SmtpSender.SendFrom( g_Options.m_Options.email.address );
         m_SmtpSender.SendTo( g_Options.m_Options.email.address );
         m_SmtpSender.Password( g_Options.m_Options.email.password );
         m_SmtpSender.Subject( "%s - Hourly Email", OptionsControl::AppName );
         m_SmtpSender.Message( "Attached are your hourly screenshots" );

         //calculate the increment with which to step through the new pictures
         uint32 step = newPics / g_Options.m_Options.email.picsPerHour;        
         if ( 0 == step ) step = 1;
         
         for ( i = 0; i < newPics; i += step )
         {
            index = g_FileExplorer.GetFileIndexByUniqueID( newPicUniqueIds[i] );            
            if ( -1 == index ) continue;

            char imageName[ MAX_PATH ] = { 0 };
            g_FileExplorer.GetFileName( index, imageName );
            
            char *pBuffer;
            sint32 fileSize;

            g_FileExplorer.GetFile( index, (void **) &pBuffer );
            fileSize = g_FileExplorer.GetFileSize( index );

            m_SmtpSender.AddAttachment( imageName, SmtpSender::AttachmentType_Jpeg, pBuffer, fileSize );

            delete [] pBuffer;
         }

         //start sending the email
         m_SmtpSender.Send( );
         
         //remember the last file id so we don't send this same batch next time
         g_Options.m_Options.email.lastFileId = newPicUniqueIds[ newPics - 1 ];
         g_Options.m_IsDirty = TRUE;
      }
   }
}

int Webserver::StartSSL( void )
{
   SYSTEMTIME systemTime;
   FILETIME fileTime;

   int result = 0;

   memset( &m_SSLDesc, 0, sizeof(m_SSLDesc) );

   char tempPath[ MAX_PATH ] = { 0 };
   char tempFile[ MAX_PATH ] = { 0 };

   GetTempPath( sizeof(tempPath), tempPath );
   GetTempFileName( tempPath, "~", 0, tempFile ); 

   do
   {
      SSL_library_init();

      SSL_load_error_strings();
      
      GetSystemTime( &systemTime );
      SystemTimeToFileTime( &systemTime, &fileTime );

      //if we see our certificate has expired
      //then we need to generate a new one
      if (CompareFileTime( &fileTime, &g_Options.m_Options.keyExpireTime) > 0 )
      {
         result = GenerateSSL( );
         if ( FALSE == result ) break;
      }

      SSL_METHOD *pMethod;

      //start up ctx
      pMethod = SSLv23_server_method( );

      m_SSLDesc.pCtx = SSL_CTX_new(pMethod);
      if ( NULL == m_SSLDesc.pCtx ) break;

      //dump the public certificate to a file and then load it
      FILE *pFile = fopen( tempFile, "wb" );
      
      fwrite( g_Options.m_Options.pCertificate, g_Options.m_Options.certificateSize, 1, pFile );

      fclose( pFile );

      result = SSL_CTX_use_certificate_file( m_SSLDesc.pCtx, tempFile, SSL_FILETYPE_PEM );
      if ( result <= 0 ) break;

      //dump the private key to a file and then load it
      pFile = fopen( tempFile, "wb" );
      
      fwrite( g_Options.m_Options.pPrivateKey, g_Options.m_Options.privateKeySize, 1, pFile );

      fclose( pFile );

      result = SSL_CTX_use_PrivateKey_file( m_SSLDesc.pCtx, tempFile, SSL_FILETYPE_PEM ); 
      if ( result <= 0 ) break;
   
      //verify they are compatible
      result = SSL_CTX_check_private_key( m_SSLDesc.pCtx );
      if ( result <= 0 ) break;
   
   } while ( 0 );

   DeleteFile( tempFile );

   return result;
}

void Webserver::ShutdownSSL( void )
{
   SSL_CTX_free( m_SSLDesc.pCtx );

   CRYPTO_cleanup_all_ex_data( );

   ERR_free_strings( );
	ERR_remove_state( 0 );
   EVP_cleanup( );

   memset( &m_SSLDesc, 0, sizeof(m_SSLDesc) );
}

int Webserver::GenerateSSL( void )
{
   int serial = rand( );
   int days   = 365;
   int result = 0;

   RSA *pRsa = NULL;
   X509 *pX509 = NULL;
   EVP_PKEY *pKey = NULL;
	BIGNUM *pBigNum = NULL;

	pBigNum = BN_new();
	BN_set_word( pBigNum, 0x10001 );

   FILE *pFile = NULL;

   char tempPath[ MAX_PATH ] = { 0 };
   char tempFile[ MAX_PATH ] = { 0 };

   GetTempPath( sizeof(tempPath), tempPath );
   GetTempFileName( tempPath, "~", 0, tempFile ); 

   do
   {
      //private key
      pRsa = RSA_new( );
      if ( NULL == pRsa ) break;

      result = RSA_generate_key_ex( pRsa, 1024, pBigNum, NULL );
      if ( result <= 0 ) break;

      pKey = EVP_PKEY_new( );
      if ( NULL == pKey ) break;

      pX509 = X509_new( );
      if ( NULL == pX509 ) break;

      result = EVP_PKEY_assign_RSA( pKey, pRsa );
      if ( result <= 0 ) break;
         
	   X509_set_version( pX509, 3 );
	   ASN1_INTEGER_set( X509_get_serialNumber(pX509), serial );
	   X509_gmtime_adj( X509_get_notBefore(pX509), 0 );
	   X509_gmtime_adj( X509_get_notAfter(pX509), (long)60*60*24*days );
	   X509_set_pubkey( pX509,pKey );

      //X509 public key
	   X509_NAME *pName = X509_get_subject_name( pX509 );

	   //This function creates and adds the entry, working out the
      //correct string type and performing checks on its length.
      result = X509_NAME_add_entry_by_txt( pName, "O", MBSTRING_ASC, (const unsigned char *) OptionsControl::CompanyName, -1, -1, 0);
      if ( result <= 0 ) break;

      result = X509_NAME_add_entry_by_txt( pName,"CN", MBSTRING_ASC, (const unsigned char *) OptionsControl::AppName, -1, -1, 0);
      if ( result <= 0 ) break;

	   result = X509_set_issuer_name( pX509, pName );
      if ( result <= 0 ) break;  

      result = X509_sign( pX509, pKey, EVP_md5( ) );
      if ( result <= 0 ) break;

      //write out the private key
      pFile = fopen( tempFile, "wb" );
      if ( NULL == pFile ) break;
      
      result = PEM_write_RSAPrivateKey( pFile, pRsa, NULL, NULL, 0, NULL, NULL );
      if ( result <= 0 ) break;
      
      int size = ftell( pFile );

      fclose( pFile );

      free( g_Options.m_Options.pPrivateKey );
      free( g_Options.m_Options.pCertificate );

      //read private key in to the options
      pFile = fopen( tempFile, "rb" );
      if ( NULL == pFile ) break;

      g_Options.m_Options.pPrivateKey    = (unsigned char *) malloc( size );
      g_Options.m_Options.privateKeySize = size;
      
      fread( g_Options.m_Options.pPrivateKey, size, 1, pFile );

      fclose( pFile );

      //write out the certificate
      pFile = fopen( tempFile, "wb" );
      if ( NULL == pFile ) break;

      result = PEM_write_X509( pFile, pX509 );
      if ( result <= 0 ) break;
      
      size = ftell( pFile );

      fclose( pFile );

      //read certificate in to the options
      pFile = fopen( tempFile, "rb" );
      if ( NULL == pFile ) break;

      g_Options.m_Options.pCertificate   = (unsigned char *) malloc( size );
      g_Options.m_Options.certificateSize= size;
      
      fread( g_Options.m_Options.pCertificate, g_Options.m_Options.certificateSize, 1, pFile );

      fclose( pFile );

      SYSTEMTIME systemTime;

      //our certificate expires one year from now
      GetSystemTime( &systemTime );   
      ++systemTime.wYear;

      SystemTimeToFileTime( &systemTime, &g_Options.m_Options.keyExpireTime );

      g_Options.m_IsDirty = TRUE;

   } while ( 0 );

   if ( pFile )
   {
      fclose( pFile );
   }

   DeleteFile( tempFile );
   
   BN_free( pBigNum );
   X509_free( pX509 );

   if ( NULL != pKey )
   {
      EVP_PKEY_free( pKey );
   }
   else
   {
      //we can't free pKey and pRsa
      //keeping pRsa does not cause a memleak
      //so it must be freed w/ pKey is freed
      RSA_free( pRsa );
   }

   return result;
}

int Webserver::GetDaysToExpire( void )
{
   FILETIME fileTime;
   GetSystemTimeAsFileTime( &fileTime );

   __int64 installTime, currentTime;
   
   memcpy( &installTime, &g_Options.m_Options.installTime, sizeof(installTime) );
   memcpy( &currentTime, &fileTime, sizeof(currentTime) );

   //currentTime is in 100 nanosecond increments so we divide by
   //10000 to get into seconds and then by 86400 which is the amont of seconds in a 24 hour day
   __int64 currentDay = (currentTime / 10000000) / 86400;
   __int64 installDay = (installTime / 10000000) / 86400;

   __int64 deltaDays = currentDay - installDay;

   //if deltaDays < 0 then currentDay was less then the install day
   //which means the product key is valid and install day is set way in the future
   if ( deltaDays < 0 ) return -1;

   deltaDays = min( deltaDays, 15 );

   return (int) (15 - deltaDays);
 }
