/*
    Copyright (C) 2010 Kasidit Yusuf.

    This file is part of "Ushahidi Symbian Uploader".

    "Ushahidi Symbian Uploader" is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    "Ushahidi Symbian Uploader" is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with "Ushahidi Symbian Uploader".  If not, see <http://www.gnu.org/licenses/>.
*/

// INCLUDE FILES
#include <e32std.h>
#include <aknnotewrappers.h>

#include "Ushahidiappui.h"
#include "Ushahidiview.h"
#include "Ushahidi.hrh"

#include "AzenqosEngineUtils.h"

#include <eikfutil.h>

_LIT(KCImageFolder,"c:\\data\\images\\");
_LIT(KEImageFolder,"e:\\images\\");

const TInt KMillion = 1000000;
const TInt KUploadIntervalSeconds = 1*60;
const TInt KMaxGPSDataAgeSeconds = 10;

_LIT(KComma,",");

CUshahidiAppUi::~CUshahidiAppUi()
{
		  delete iPeriodic;
		  delete iCDataImgFolderWatcher;
		  delete iEImgFolderWatcher;
    	  delete iAzqInternalGPSReader;
};

void CUshahidiAppUi::ConstructL()
    {
    // Initialise app UI
    BaseConstructL(EAknEnableSkin);

    iAppView = CUshahidiView::NewL();


    // Transfer ownership to base class
    AddViewL( iAppView );


    SetDefaultViewL( *iAppView );


    ///construct engine stuff
    TRAPD(err,
    	iAzqInternalGPSReader = CAzqInternalGPSReader::NewL(*this);
    	iAzqInternalGPSReader->StartL();
    	);

    	if(err!=KErrNone)
    		{
    		_LIT(msg,"Can't find/start internal GPS");
    		CAknWarningNote* informationNote = new (ELeave) CAknWarningNote(ETrue);
    		informationNote->ExecuteLD(msg);
    		delete iAzqInternalGPSReader;
    		iAzqInternalGPSReader = NULL;
    		}


	iCDataImgFolderWatcher = CFolderWatcher::NewL(KCImageFolder,*this);
	iEImgFolderWatcher = CFolderWatcher::NewL(KEImageFolder,*this);

	//TODO: add video uploading

	iCDataImgFolderWatcher->StartL();
	iEImgFolderWatcher->StartL();

	TCallBack aCallBack(OnUploadTimerCallback,this);
	iPeriodic = CPeriodic::NewL(EPriorityHigh);
	iPeriodic->Start(KUploadIntervalSeconds*KMillion,KUploadIntervalSeconds*KMillion,aCallBack);
    ///////////////////
    }

// -----------------------------------------------------------------------------
// CUshahidiAppUi::HandleCommandL()
// Takes care of command handling.
// -----------------------------------------------------------------------------
//
void CUshahidiAppUi::HandleCommandL( TInt aCommand )
    {
    switch ( aCommand )
        {
		case EUshahidiCommandStopAndExit:
        case EEikCmdExit:
        case EAknSoftkeyExit:
            Exit();
            break;
        default:
            break;
        }
    }

// -----------------------------------------------------------------------------
// CUshahidiAppUi::HandleResourceChangeL()
// Called by framework when layout is changed.
// -----------------------------------------------------------------------------
//
void CUshahidiAppUi::HandleResourceChangeL( TInt aType )
    {
    CAknAppUi::HandleResourceChangeL( aType );

    if ( aType==KEikDynamicLayoutVariantSwitch )
        {
        iAppView->HandleSizeChange(aType);
        }
    }




void CUshahidiAppUi::OnGPSStateUpdate(const TDesC& state, TAzqGPSData& aGPSData)
	{
		iGpsStateStr = state;
		iGPSData = aGPSData;
		iLastGpsDataTime.HomeTime();

		/*
		if(iGPSData.iLat.Length()==0)
			iZRecView->UpdateGps(KGPSWaitStr);

		else
			iZRecView->UpdateGps(KGPSReadyStr);
			*/
		/*
		TRAPD(terr,

						TUid titlePaneUid;
			    		titlePaneUid.iUid = EEikStatusPaneUidTitle;

						CEikStatusPane* statusPane = StatusPane();

			   			CEikStatusPaneBase::TPaneCapabilities subPane =
							statusPane->PaneCapabilities(titlePaneUid);

			            // if we can access the title pane
						if (subPane.IsPresent() && subPane.IsAppOwned())
							{

			    			CAknTitlePane* titlePane = (CAknTitlePane*) statusPane->ControlL(titlePaneUid);

			                // read the title text from the resource file



			    			titlePane->DrawNow();

							}

						);*/
	}


void CUshahidiAppUi::GenerateLogNameL(const TDesC& aFolderPath,TTime &aTestTime,TDes& aLogFile)
	{
		aLogFile.Zero();


		RFs fs = CCoeEnv::Static()->FsSession();
        User::LeaveIfError(fs.Connect());
        CleanupClosePushL(fs);

        //////now we store all longs in same directory
        //TFileName dir(aTestType);
        //dir+=KBackSlash;
        //////

        TFileName dir;
        dir.Insert(0,aFolderPath);



        TInt err = fs.MkDirAll(dir);
    	if(err == KErrAlreadyExists)
			err = KErrNone;
		User::LeaveIfError(err);
		CleanupStack::PopAndDestroy();


		TDateTime date = aTestTime.DateTime();

		_LIT(KFormatTxt,"%02d%02d%02d-%02d%02d%02d");
		aLogFile.Format(KFormatTxt,date.Year()%100,date.Month()+1,date.Day()+1,date.Hour(),date.Minute(),date.Second());
		aLogFile.Append(_L(".csv"));
		aLogFile.Insert(0,dir);
	}


void CUshahidiAppUi::LeaveIfPathDiskFullL(RFs &aFs, const TDesC& aLogFile, TInt aBytesToWrite)
	{
 			if(aLogFile[0] == 'c' || aLogFile[0] == 'C')
        		{
        			if(SysUtil::FFSSpaceBelowCriticalLevelL(&aFs,aBytesToWrite))
        				User::Leave(KErrDiskFull);
        		}
        	else
        		{
        			if(SysUtil::MMCSpaceBelowCriticalLevelL(&aFs,aBytesToWrite))
        				User::Leave(KErrDiskFull);
        		}
	}

void CUshahidiAppUi::CreateInfoCsv(TDes& aFileName)
{
					//csv format: time,phoneinfo,cellid,lat,lon


						TTime now;
						now.HomeTime();

						 TBuf<1024> log;
						 TAzenqosEngineUtils::MakeTimeStrMilli(now,log);
					     _LIT(Kcomma,",");
					     _LIT(KSemicolon,";");

					   	TTimeIntervalSeconds seconds(KMaxGPSDataAgeSeconds+1); //invalid by default

						 TInt err = now.SecondsFrom(iLastGpsDataTime,seconds);

						 if(iAzqInternalGPSReader && err == KErrNone && seconds <= TTimeIntervalSeconds(KMaxGPSDataAgeSeconds) )
							 {
							 log+=iGPSData.iLat;
							 }
						 log+=KComma;
						 if(iAzqInternalGPSReader && err == KErrNone && seconds <= TTimeIntervalSeconds(KMaxGPSDataAgeSeconds) )
							 {
							 log+=iGPSData.iLon;
							 }
						 else
							 {
								/* _LIT(msg,"Note: GPS data not available");
								CAknWarningNote* informationNote = new (ELeave) CAknWarningNote(ETrue);
								informationNote->ExecuteLD(msg);*/
							 }

						 log += KComma;
						 log += aFileName;

						 {//add how old is the lat lon data
						 log += KComma;
						 TBuf<64> secondsstr;
						 TInt nseconds = -1;
						 nseconds = seconds.Int();

						 if(err == KErrNone) //if seconds is true
							 secondsstr.Num(nseconds);
						 else
							 secondsstr = _L("NA");

						 log += secondsstr;
						 }
						 ////////////////////////////////

						RFs fs = CCoeEnv::Static()->FsSession();
			        	User::LeaveIfError(fs.Connect());
			        	CleanupClosePushL(fs);
			        	RFile file;

			        	err=KErrNone;

			        	TFileName folder;

			        	folder = _L("list\\");
			        	TAzenqosEngineUtils::CompleteWithPrivatePathL(folder);

			        	fs.MkDirAll(folder);//make folder if not exist

			        	TFileName logfn;

						GenerateLogNameL(folder, now, logfn);

			        	EikFileUtils::DeleteFile(logfn);

			        	User::LeaveIfError(file.Create(fs,logfn,EFileWrite));

			        		CleanupClosePushL(file);
							//TInt offset = 0;
							TInt fsize=0;
							file.Size(fsize);
							//file.Seek(ESeekEnd,offset);

							HBufC8* output = HBufC8::NewLC(log.Length());
							(output->Des()).Copy(log);

							//////////////////////check space
							LeaveIfPathDiskFullL(fs,logfn,(*output).Length()+1/*endl*/);
							/////////////////////

							file.Write(*output);
							_LIT8(KEndl8,"\n");
							file.Write(KEndl8());
							file.Flush();
							CleanupStack::PopAndDestroy(output);
							CleanupStack::PopAndDestroy();//file

						CleanupStack::PopAndDestroy();//fs



						TBuf<128> rembuf;
								int remaining = 0;

								TRAPD(rerr, remaining = GetNWatingUploadL(););

								/*if(State>=ESendingTYPE)
									remaining = (iUploadList.Count()+1);
								else
									remaining = (iUploadList.Count());
								*/
								rembuf.Format(_L("%d left to upload... next upload within 2 minutes."),remaining);


								//iZRecView->UpdateFtpState(rembuf);

}

void CUshahidiAppUi::OnFolderChange(const TDesC& aFolder)
	{
	_LIT(KJpgMatch,"*.jpg");
	_LIT(KAmrMatch,"*.amr");

	AddNewFiles(aFolder,KJpgMatch);
	AddNewFiles(aFolder,KAmrMatch);

	}



void CUshahidiAppUi::AddNewFiles(const TDesC& folder, const TDesC& match)
	{
				RFs fs = iCoeEnv->FsSession();
			    User::LeaveIfError(fs.Connect());
			    CleanupClosePushL(fs);

			 //how to find file list: http://developer.uiq.com/devlib/uiq_31/sdkdocumentation/doc_source/doc_source/faqSDK/faq_0617.html

			 //_LIT(KNtrFolderPath,"C:\\NTR\\");

			    //_LIT(KFindFormat,"*.ntr");
			    TFindFile search(fs); // 1
			    CDir* aFileList =NULL;

			    TInt err=0;

			    //err = search.FindByPath(_L("*.*"),folder,aFileList); // 3
			    TUint mask = KEntryAttNormal|KEntryAttDir;
			    err = fs.GetDir(folder,mask,ESortByName, aFileList);

			    		if(err == KErrNone && aFileList)
			    			{

			    			//	TRAPD(err,

			    					for(TInt i=0;i<aFileList->Count();i++)
			    						{

			    				            TFileName fp = folder;
			    				            fp += (*aFileList)[i].iName;

			    				            if(fp.Length()<5)
			    				            	continue;

			    				            if((*aFileList)[i].IsDir())
												{
													if( fp.Length() > 4  && fp!= folder && fp.Right(1) != _L("."))
														{
															/////////////////////////////////////////////////
															//call this func recursively - in symbian 9.3 (s60 fp2) the images are in another subfolder
															fp+=_L("\\");
															AddNewFiles(fp,match);
															////////////////////////////////////////
														}
												}
			    				            else if(fp.Right(4) == match.Right(4))
			    				            	{
													/*////////check if already in list
													TBool already = EFalse;
													for(TInt j=0;j<iUploadList.Count();j++)
														{
															if(iUploadList[j].iFile == fullentry.FullName())
																{
																already = ETrue;
																break;
																}
														}
													////////////////////////////*/

													TTime now;
													now.UniversalTime();//sinc RFs moditied() time is universal
													TTime ft=0;
													if(fs.Modified(fp,ft) == KErrNone)
														{
															TTimeIntervalSeconds min(10);
															TTimeIntervalSeconds diff(0);
															TInt terr = now.SecondsFrom(ft,diff);
															if(terr == KErrNone && diff < min )
																{
																	CreateInfoCsv(fp);
																}
														}
			    				            	//delete gz files remaining from any previous uploads
			    				            	//the->iFileList.AppendL(fullentry);

			    				            	}

			    						}
			    				//);
			    			}

		    	delete aFileList; // 9
			    aFileList = NULL;



			    CleanupStack::PopAndDestroy();//fs

	}


TInt CUshahidiAppUi::OnUploadTimerCallback(TAny* caller)
	{
		CUshahidiAppUi* that = (CUshahidiAppUi*) caller;

		//get a list of all files in c:\\data\\images and e:\\images thare are not already in list and are older than start time

		//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("OnUploadTimerCallback0"));
		that->UploadAllInList();
		//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("OnUploadTimerCallback1"));

		return 1;
	}

TInt CUshahidiAppUi::GetNWatingUploadL()
	{
			TFileName folder;
			folder = _L("list\\");
			TAzenqosEngineUtils::CompleteWithPrivatePathL(folder);
			RFs fs = iCoeEnv->FsSession();
			User::LeaveIfError(fs.Connect());
			CleanupClosePushL(fs);
			TFindFile search(fs);

			CDir* aFileList =NULL;
			TInt err=0;

			_LIT(KCsvMatch,"*.csv");

			err = search.FindWildByDir(KCsvMatch,folder,aFileList);

			TInt ret = 0;

			while (err==KErrNone)
			{
				if(err == KErrNone && aFileList)
					{
					 ret += aFileList->Count();
					}

					if(err!=KErrNone)
					{
					break;
					}

			delete aFileList; // 9
			aFileList = NULL;
			err=search.FindWild(aFileList); // more, more! // 10
			}

			CleanupStack::PopAndDestroy();//fs

			//027252525 smt **41

			return ret;

	}

void CUshahidiAppUi::HandleWsEventL(const TWsEvent &aEvent, CCoeControl *aDestination)
	{
		switch (aEvent.Type())
		{
			case KAknUidValueEndKeyCloseEvent:
			{
				TApaTask task(iEikonEnv->WsSession( ));
				task.SetWgId(CEikonEnv::Static()->RootWin().Identifier());
				task.SendToBackground();
			}
			break;

			default:
			{
			CAknAppUi::HandleWsEventL(aEvent, aDestination);
			}
		}
	}



void CUshahidiAppUi::UploadAllInList()
	{
	//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("UploadAllInList 0"));
	//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("UploadAllInList 1"));

		if(iUploadList.Count()>0) //in case iazqftpengine is (is actually busy) going to run but not active after connect complete
			return;

		//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("UploadAllInList 2"));

		//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("UploadAllInList 3"));

		//iUploadCsvArray->Reset();
		iUploadList.Reset();

		TFileName folder;
		folder = _L("list\\");
		TAzenqosEngineUtils::CompleteWithPrivatePathL(folder);
		RFs fs = iCoeEnv->FsSession();
		User::LeaveIfError(fs.Connect());
		CleanupClosePushL(fs);
		TFindFile search(fs);
		CDir* aFileList =NULL;
		TInt err=0;

		//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("UploadAllInList 4"));

		_LIT(KCsvMatch,"*.csv");

		err = search.FindWildByDir(KCsvMatch,folder,aFileList);

		//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("UploadAllInList 5"));

		while (err==KErrNone)
		{
				if(err == KErrNone && aFileList)
					{
						aFileList->Sort(ESortByName|EDescending);
						for(TInt i=0;i<aFileList->Count();i++)
							{
							//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("UploadAllInList 6"));

								TParse fullentry;
								fullentry.Set((*aFileList)[i].iName,& search.File(),NULL); // 5,6,7

								//TDebugLog::LogToFile(_L("c:\\ir.log"),fullentry.FullName());

								if(!((*aFileList)[i].IsDir()))
									{
										//add to list
									TUploadFile csvupload;

									csvupload.iFile = fullentry.FullName();
									csvupload.iNameAs = fullentry.NameAndExt();

									//read csvfile to ram
									TBool csvvalid = ETrue;

									TRAPD(ferr,
									RFile file;
									User::LeaveIfError(file.Open(fs,fullentry.FullName(),EFileRead));
									CleanupClosePushL(file);
									TInt sz=0;
									User::LeaveIfError(file.Size(sz));
									if(sz==0)
									{
										//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("size 0 so leave"));
										User::Leave(KErrGeneral);
									}
									HBufC8* buf = HBufC8::NewLC(sz);
									TPtr8 ptr(buf->Des());
									file.Read(ptr);

									//parse buf
									TPtrC8 cur(0,0);
									TPtrC8 rem(0,0);

									rem.Set(buf->Des());

									////TDebugLog::LogToFile(_L("c:\\ir.log"),rem);

									if(TAzenqosEngineUtils::TokenizeCSV8(*buf,cur,rem))
										{
										//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("tok0"));

										TPtrC8 prelast(0,0);
											while(TAzenqosEngineUtils::TokenizeCSV8(rem,cur,rem))
												prelast.Set(cur);

											rem.Set(cur);

											if(rem.Length()>4 && rem.Length()<128)
												{

												//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("tok2"));

												//in list upload csv first, then media file

											iUploadList.Append(csvupload);

											//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("tok3"));

											TUploadFile fileupload;

											fileupload.iFile.Copy(rem);

											////TDebugLog::LogToFile(_L("c:\\ir.log"),);


											fileupload.iNameAs = fullentry.NameAndExt();
											TBuf<4> extnodot;
											extnodot.Copy(rem.Right(3));
											fileupload.iNameAs.Replace(fileupload.iNameAs.Length()-3,3,extnodot);
											fileupload.iCsvRefFile = csvupload.iFile;
											//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("UploadAllInList preappend"));
											iUploadList.Append(fileupload);
											//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("UploadAllInList post append"));

												}
											else
												{
												csvvalid = EFalse;
												//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("csvvalid = EFalse inner"));
												}



										}
									else
										{
										csvvalid = EFalse;
										//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("csvvalid = EFalse outer"));
										}

									CleanupStack::PopAndDestroy(2);//buf,file
									);

									//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("trap ferr"),ferr);

									//should delete here because csv file is now closed above
									if(csvvalid == EFalse)
										EikFileUtils::DeleteFile(fullentry.FullName());


									}
							}

					}

				if(err!=KErrNone)
				{
				break;
				}
		delete aFileList; // 9
		aFileList = NULL;
		err=search.FindWild(aFileList); // more, more! // 10
		}

		CleanupStack::PopAndDestroy();//fs

		//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("check ull count "),iUploadList.Count());

		if(iUploadList.Count()==0)
			return;

		//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("start ftp engine"));

		////////////////start ftp engine
		iUploadingFile.iFile.Zero();
		iUploadingFile.iNameAs.Zero();

		//TODO: start upload engine

		////////////

		//TDebugLog::LogToFile(_L("c:\\ir.log"),_L("start ftp engine done"));

	}

// End of File
