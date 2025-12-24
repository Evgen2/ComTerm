 /* Comterm.cpp */
 /* Simple COM-port Terminal for Windows */
 /* (ñ) Evgen2 */

#include "stdafx.h"
#include <windows.h>
#include <string.h>
#include <conio.h>
#include <time.h>

void SetColor(int text, int background)
{   HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hStdOut, (WORD)((background << 4) | text));
}

#define FATAL_COLOR         {SetColor(12,0);}
#define WARNING_COLOR       {SetColor(14,0);}
#define NORMAL_COLOR        {SetColor(15,0);}
#define ATTENSION_COLOR     {SetColor(13,0);}

DCB DCB1;

int ComControl(int NComPort, int Bitrate, int fw, int logmode);
HANDLE initComPort(int NComPort, int Bitrate, int fw);
int Comgets(HANDLE Hcom,char *Buf,int buflen);

char LogFileName[]="comTerm.log";

FILE *fpLog = NULL; 
int echomode  = 0;

int _tmain(int npar, _TCHAR* par[])
{  int nport=1,Bitrate=9600, fw=0, logmode=0, rc;
    WARNING_COLOR; printf("ComTerm\nExit=Crtl+Break\n"); NORMAL_COLOR;

    if(npar > 1)
    { if( !_stricmp(par[1],"help") ||  (*par[1] == '-' && *(par[1]+1) == '?') )
      {  printf("Usage: comTerm Nport [Bitrate [log] [F=%%x]] [echo]\n");   
         exit(0);
      }
      rc = sscanf(par[1],"%i",&nport);
      printf("Port=%i",nport); 
      if(npar>0 && !_stricmp(par[npar-1],"echo")) 
      {  echomode=1;
        npar--;
        printf ("echo mode\n");
      }
       if(npar > 2)
        { rc = sscanf(par[2],"%i",&Bitrate);
           printf(" Bitrate=%i",Bitrate); 
          if(npar > 3)
          {  if(!_stricmp(par[3],"log")) logmode=1;
             else 
               if(*par[3] == 'F' && *(par[3]+1) == '=')
             {  rc = sscanf(par[3]+2,"%x",&fw);
                printf(" FW=%x\n",fw); 
             }
          }
          if(npar > 4)
          {
             if(*par[4] == 'F' && *(par[4]+1) == '=')
             {  rc = sscanf(par[4]+2,"%x",&fw);
                printf(" FW=%x\n",fw); 
             }
          }
       }   
    }  

    ComControl(nport,Bitrate,fw, logmode);
	return 0;
}

 /* terminal */
int ComControl(int NComPort, int Bitrate, int fw, int logmode)
{  int rc,rc1, t0=0,t1, isKey=0, hexmode=0;
    int i, nbytes, ierr, Nunit=0;
    HANDLE Hcom;
    char str[520];

    Hcom = initComPort(NComPort, Bitrate, fw);
    if(Hcom == NULL) return 0;

	fpLog = fopen(LogFileName,"wb");
    if(fpLog)
		printf("Logfile=%s\n", LogFileName);
    if(logmode)    
       printf("Log file write only mode\n");
M0:
    nbytes = Comgets(Hcom,str,512);
    if(nbytes > 0)
     {  static int total=0, t0=0, t,t1, t10=0;  
        int needflush=0;    
        if(logmode)
        {  if(total==0) t0 = clock();
           else
           {  t = clock()-t0;
              t1 = t/1000;
              if(t1!=t10)
              {  // printf("\r%sBytes/s=%.3f %i%s\n", 
                 //   WARNING_COLOR,((double)(total))/t1,nbytes,NORMAL_COLOR);
                 printf("%i bytes recived\n",total); 
                 t10 = t1;
              }
           }
           total += nbytes;
        }   
          str[nbytes+1] = 0;
          if(hexmode)
          {   if(!logmode)    
                    for(i=0; str[i]; i++)  printf("%3x ",str[i]); 
          } else {
             if(str[nbytes-1] == '\r') 
             {   str[nbytes] = '\n';
                 needflush++;
                 nbytes++;
             }     
	          if(!logmode)    
                 printf("%s",str);
          } 
          if(needflush)
          {   fflush(stdout);
              needflush = 0;
          }
          if(fpLog)
             fwrite(str,1, nbytes, fpLog);
     } 

    t0 = clock();
    for(;;)
    { static int iii=0, rr;
       isKey = 0;
	   rr = _kbhit();
      if(rr)
      {   rc = _getch();
          if(_kbhit())
          {  rc1 = _getch();
            if(rc1){ rc += 100; isKey = 1; }
          }
          
		  if(echomode)          
		  {   if(isKey)          
			  {  //printf("%c",rc-100); 
			  } else {  
				printf("%c",rc); 
			  }   
			 fflush(stdout);
		  }

        break;
       }
      t1 = clock();
      if(t1-t0 > 2)
      {  rc =0; break;
      } else {
          Sleep(1); 
	  }
	}

   if(!isKey)
   {    if(rc)
        { DWORD  numbytes_ok; 
		  if(rc == '\r')
		  {	str[0]= rc;
			str[1]= '\n';
			str[2]= 0;
			rc =  WriteFile(Hcom, str, 2, &numbytes_ok, NULL); 
		  } else {
           str[0]= rc;
           str[1]= 0;
		   rc =  WriteFile(Hcom, str, 1, &numbytes_ok, NULL); 
		  }
		  if (rc == 0) //error close port
		  { ierr = GetLastError();
			printf("WriteFile Error: %i\n",ierr);
		  }
        }
   } 
   
 goto M0;

   rc = CloseHandle(Hcom);
   if (rc == 0) //error close port
   { ierr = GetLastError();
		printf("Open port Error: %i\n",ierr);
		exit(1);
  }

   return 0;
} 


HANDLE initComPort(int NComPort, int Bitrate, int fw)
{   char devName[16];
    int ierr, rc;
  HANDLE hf=NULL;          /* File handle for the device           */

  USHORT usBPS = 2400;/* Bit rate to set the COM port to      */
  ULONG  ulParmLen= 2;/* Maximum size of the parameter packet */
  CHAR COMSETTING[256]; //="Com1: baud=1200 parity=N data=8 stop=1";

  if(NComPort < 10)
  {	 sprintf(devName,"COM%i:",NComPort);
     sprintf(COMSETTING,"COM%i: baud=%i parity=N data=8 stop=1", NComPort,Bitrate);
  } else {
		sprintf(devName,"\\\\.\\COM%i",NComPort);
        sprintf(COMSETTING,"baud=%i parity=N data=8 stop=1", Bitrate);
  }

  hf = CreateFile(devName, GENERIC_READ|GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, NULL,NULL);           

  if (hf==INVALID_HANDLE_VALUE)
  { 	ierr = GetLastError();
		printf("Open port Error: %i\n",ierr);
		if(ierr == 2)
		      printf("5 = ERROR_FILE_NOT_FOUND\n");
		else if(ierr == 5)
		      printf("5 = ERROR_ACCESS_DENIED\n");
		exit(1);
  } else { 
      printf("Open of %s Ok\n",devName);
  }

    rc = BuildCommDCB(COMSETTING,&DCB1);
	if (rc == 0)	//error DCB
    { ierr = GetLastError();
	 	printf("DCB Structure ERROR: %i\n",ierr);
		exit(1);
   }
   rc = SetCommState(hf,&DCB1);
  if (rc == 0)	//error SetCom
    { ierr = GetLastError();
	 	printf("SetComm Function ERROR: %i\n",ierr);
		exit(1);
   }
  printf("Hcom=%x\n", hf);
  return hf;
} 

int Comgets(HANDLE Hcom,char *Buf,int buflen)
{	int rc, ierr;
    int nbytes,nbytesRead;
	DWORD numbytes_ok, temp; 
    unsigned long int dl;
    COMSTAT ComState;

    dl = 2;
	rc = ClearCommError(Hcom, &temp, &ComState); 
  if (rc == 0)	//error SetCom
  {	ierr = GetLastError();
	printf("ClearCommError Function ERROR: %i\n",ierr);
	exit(1);
  }
  nbytesRead = 0;
  nbytes = ComState.cbInQue;
  if(nbytes)
  {	if(nbytes >= buflen)
			nbytes = buflen - 1;
	rc = ReadFile(Hcom, Buf, nbytes, &numbytes_ok,  NULL);
	if (rc == 0) //error close port
	{	ierr = GetLastError();
		printf("ReadFile Error: %i\n",ierr);
		printf("Hcom=%x\n", Hcom);  
	}
	nbytesRead =  numbytes_ok;
    Buf[nbytesRead] = 0;
  }
  return nbytesRead;
}
