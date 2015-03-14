// swad_profile.c: user's public profile

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2015 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General 3 License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*****************************************************************************/
/*********************************** Headers *********************************/
/*****************************************************************************/

#include <linux/stddef.h>	// For NULL

#include "swad_config.h"
#include "swad_database.h"
#include "swad_global.h"
#include "swad_nickname.h"
#include "swad_parameter.h"
#include "swad_privacy.h"
#include "swad_role.h"
#include "swad_text.h"
#include "swad_theme.h"
#include "swad_user.h"

/*****************************************************************************/
/****************************** Public constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private constants *****************************/
/*****************************************************************************/

/*****************************************************************************/
/****************************** Internal types *******************************/
/*****************************************************************************/

struct UsrFigures
  {
   struct DateTime FirstClickTime;	//   0 ==> unknown first click time of user never logged
   int NumDays;				//  -1 ==> not applicable
   long NumClicks;			// -1L ==> unknown number of clicks
   long NumFileViews;			// -1L ==> unknown number of file views
   long NumForPst;			// -1L ==> unknown number of forum posts
   long NumMsgSnt;			// -1L ==> unknown number of messages sent
  };

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Internal global variables *************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Private prototypes ****************************/
/*****************************************************************************/

static void Prf_ShowUserProfile (void);
static void Prf_ShowDetailsUserProfile (const struct UsrData *UsrDat);
static void Prf_ShowHistoricUserProfile (const struct UsrData *UsrDat);
static void Prf_GetUsrFigures (long UsrCod,struct UsrFigures *UsrFigures);
static unsigned long Prf_GetRankingNumClicks (long UsrCod);
static unsigned long Prf_GetNumUsrsWithNumClicks (void);
static unsigned long Prf_GetRankingNumClicksPerDay (long UsrCod);
static unsigned long Prf_GetNumUsrsWithNumClicksPerDay (void);
static void Prf_GetFirstClickFromLogAndStoreAsUsrFigure (long UsrCod);
static void Prf_GetNumClicksAndStoreAsUsrFigure (long UsrCod);
static void Prf_GetNumFileViewsAndStoreAsUsrFigure (long UsrCod);
static void Prf_GetNumForPstAndStoreAsUsrFigure (long UsrCod);
static void Prf_GetNumMsgSntAndStoreAsUsrFigure (long UsrCod);
static void Prf_ResetUsrFigures (struct UsrFigures *UsrFigures);
static void Prf_CreateUsrFigures (long UsrCod,const struct UsrFigures *UsrFigures);
static bool Prf_CheckIfUsrFiguresExists (long UsrCod);

/*****************************************************************************/
/************************** Get public profile URL ***************************/
/*****************************************************************************/

char *Prf_GetURLPublicProfile (char *URL,const char *NicknameWithoutArroba)
  {
   extern const char *Txt_STR_LANG_ID[Txt_NUM_LANGUAGES];

   /***** Build URL using nickname *****/
   sprintf (URL,"%s/%s?usr=@%s",
	    Cfg_HTTPS_URL_SWAD_CGI,Txt_STR_LANG_ID[Gbl.Prefs.Language],
	    NicknameWithoutArroba);

   return URL;
  }

/*****************************************************************************/
/************************** Request a user's profile *************************/
/*****************************************************************************/

void Prf_RequestUserProfile (void)
  {
   extern const char *Txt_View_public_profile;
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_Nickname;
   extern const char *Txt_Continue;

   /***** Start frame *****/
   Lay_StartRoundFrameTable10 (NULL,2,Txt_View_public_profile);
   fprintf (Gbl.F.Out,"<tr>"
                      "<td>");

   /***** Form to request user's @nickname *****/
   Act_FormStart (ActSeePubPrf);
   fprintf (Gbl.F.Out,"<div class=\"%s\" style=\"text-align:center;\">"
                      "%s: "
                      "<input type=\"text\" name=\"usr\""
                      " size=\"20\" maxlength=\"%u\" value=\"@%s\" />"
                      "</div>",
            The_ClassFormul[Gbl.Prefs.Theme],
            Txt_Nickname,
            Nck_MAX_BYTES_NICKNAME_WITH_ARROBA,
            Gbl.Usrs.Me.UsrDat.Nickname);	// If no user logged ==> nickname is empty

   /***** Send button*****/
   Lay_PutSendButton (Txt_Continue);
   Act_FormEnd ();

   /***** End frame *****/
   fprintf (Gbl.F.Out,"</td>"
                      "</tr>");
   Lay_EndRoundFrameTable10 ();
  }

/*****************************************************************************/
/**************** Get user's code and show a user's profile ******************/
/*****************************************************************************/

void Prf_GetUsrCodAndShowUserProfile (void)
  {
   char Nickname[Nck_MAX_BYTES_NICKNAME_WITH_ARROBA + 1];
   long OtherUsrCod;

   /***** Get user from nickname *****/
   if (Gbl.Usrs.Other.UsrDat.UsrCod < 0)
     {
      Par_GetParToText ("usr",Nickname,Nck_MAX_BYTES_NICKNAME_WITH_ARROBA);
      if ((OtherUsrCod = Nck_GetUsrCodFromNickname (Nickname)) > 0)
	{
	 Gbl.Usrs.Other.UsrDat.UsrCod = OtherUsrCod;
	 Gbl.CurrentAct = ActSeePubPrf;
	}
      else
	 Usr_GetParamOtherUsrCodEncrypted ();
     }

   /***** Show user's profile *****/
   Prf_ShowUserProfile ();
  }

/*****************************************************************************/
/*************************** Show a user's profile ***************************/
/*****************************************************************************/

static void Prf_ShowUserProfile (void)
  {
   extern const char *Txt_User_not_found_or_you_do_not_have_permission_;
   bool Error = false;

   /***** Check if user exists and get his data *****/
   if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat))        // Existing user
     {
      /***** Check if I can see the public profile *****/
      if (Pri_ShowIsAllowed (Gbl.Usrs.Other.UsrDat.ProfileVisibility,Gbl.Usrs.Other.UsrDat.UsrCod))
	{
	 if (Gbl.CurrentCrs.Crs.CrsCod > 0)	// Course selected
	   {
	    /* Get user's role in current course */
	    Gbl.Usrs.Other.UsrDat.RoleInCurrentCrsDB = Rol_GetRoleInCrs (Gbl.CurrentCrs.Crs.CrsCod,Gbl.Usrs.Other.UsrDat.UsrCod);

	    /* Get if user has accepted enrollment in current course */
	    Gbl.Usrs.Other.UsrDat.Accepted = Usr_GetIfUserHasAcceptedEnrollmentInCurrentCrs (Gbl.Usrs.Other.UsrDat.UsrCod);
	   }

	 fprintf (Gbl.F.Out,"<div style=\"margin:0 auto;\">"
	                    "<table style=\"margin:0 auto;\">"
	                    "<tr>"
	                    "<td style=\"text-align:right;"
	                    " vertical-align:top;\">");

	 /***** Common record *****/
	 Rec_ShowSharedUsrRecord (Rec_RECORD_PUBLIC,&Gbl.Usrs.Other.UsrDat);

	 /***** Show details of user's profile *****/
	 Prf_ShowDetailsUserProfile (&Gbl.Usrs.Other.UsrDat);

	 fprintf (Gbl.F.Out,"</td>"
	                    "<td style=\"text-align:left;"
	                    " vertical-align:top; padding-left:4px;\">");

	 /***** Show historic user's profile *****/
	 Prf_ShowHistoricUserProfile (&Gbl.Usrs.Other.UsrDat);

	 fprintf (Gbl.F.Out,"</td>"
	                    "</tr>"
	                    "</table>"
	                    "</div>");
	}
      else
	 Error = true;
     }
   else
      Error = true;

   if (Error)
     {
      /***** Show error message *****/
      Lay_ShowAlert (Lay_WARNING,Txt_User_not_found_or_you_do_not_have_permission_);

      /***** Request nickname again *****/
      Prf_RequestUserProfile ();
     }
  }

/*****************************************************************************/
/******************** Change my public profile visibility ********************/
/*****************************************************************************/

void Prf_ChangeProfileVisibility (void)
  {
   extern const char *Pri_VisibilityDB[Pri_NUM_OPTIONS_PRIVACY];
   extern const char *Txt_The_visibility_of_your_public_profile_has_changed;
   char Query[128];

   /***** Get param with public/private photo *****/
   Gbl.Usrs.Me.UsrDat.ProfileVisibility = Pri_GetParamVisibility ();

   /***** Store public/private photo in database *****/
   sprintf (Query,"UPDATE usr_data SET ProfileVisibility='%s'"
	          " WHERE UsrCod='%ld'",
            Pri_VisibilityDB[Gbl.Usrs.Me.UsrDat.ProfileVisibility],
            Gbl.Usrs.Me.UsrDat.UsrCod);
   DB_QueryUPDATE (Query,"can not update your preference about public profile visibility");

   /***** Show alert *****/
   Lay_ShowAlert (Lay_SUCCESS,Txt_The_visibility_of_your_public_profile_has_changed);

   /***** Show form again *****/
   Pri_EditMyPrivacy ();
  }

/*****************************************************************************/
/********************** Show details of user's profile ***********************/
/*****************************************************************************/

static void Prf_ShowDetailsUserProfile (const struct UsrData *UsrDat)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   // extern const char *Txt_Figures;
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_ROLES_SINGUL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_ROLES_PLURAL_abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_course;
   extern const char *Txt_courses;
   extern const char *Txt_Files;
   extern const char *Txt_file;
   extern const char *Txt_files;
   extern const char *Txt_public_FILES;
   unsigned NumCrssUsrIsTeacher;
   unsigned NumCrssUsrIsStudent;
   unsigned NumStds;
   unsigned NumTchs;
   unsigned NumFiles;
   unsigned NumPublicFiles;

   /***** Start table *****/
   fprintf (Gbl.F.Out,"<table class=\"TABLE10 CELLS_PAD_2\">"
	              "<tr>");

   /***** Number of courses in which the user is teacher or student *****/
   if ((NumCrssUsrIsTeacher = Usr_GetNumCrssOfUsrWithARole (UsrDat->UsrCod,Rol_ROLE_TEACHER)))
     {
      NumTchs = Usr_GetNumUsrsInCrssOfAUsr (UsrDat->UsrCod,Rol_ROLE_TEACHER,Rol_ROLE_TEACHER);
      NumStds = Usr_GetNumUsrsInCrssOfAUsr (UsrDat->UsrCod,Rol_ROLE_TEACHER,Rol_ROLE_STUDENT);
      fprintf (Gbl.F.Out,"<td class=\"%s\""
			 " style=\"text-align:right; vertical-align:top;\">"
                         "%s"
			 "</td>"
			 "<td class=\"DAT\""
			 " style=\"text-align:left; vertical-align:top;\">"
			 "%u&nbsp;%s<br />"
			 "%u&nbsp;%s<br />"
			 "%u&nbsp;%s"
			 "</a>"
			 "</td>",
	       The_ClassFormul[Gbl.Prefs.Theme],
	       Txt_ROLES_SINGUL_Abc[Rol_ROLE_TEACHER][UsrDat->Sex],
	       NumCrssUsrIsTeacher,
	       (NumCrssUsrIsTeacher == 1) ? Txt_course :
		                            Txt_courses,
	       NumTchs,
	       (NumTchs == 1) ? Txt_ROLES_SINGUL_abc[Rol_ROLE_TEACHER][Usr_SEX_UNKNOWN] :
		                Txt_ROLES_PLURAL_abc[Rol_ROLE_TEACHER][Usr_SEX_UNKNOWN],
	       NumStds,
	       (NumStds == 1) ? Txt_ROLES_SINGUL_abc[Rol_ROLE_STUDENT][Usr_SEX_UNKNOWN] :
		                Txt_ROLES_PLURAL_abc[Rol_ROLE_STUDENT][Usr_SEX_UNKNOWN]);
     }
   if ((NumCrssUsrIsStudent = Usr_GetNumCrssOfUsrWithARole (UsrDat->UsrCod,Rol_ROLE_STUDENT)))
     {
      NumTchs = Usr_GetNumUsrsInCrssOfAUsr (UsrDat->UsrCod,Rol_ROLE_STUDENT,Rol_ROLE_TEACHER);
      NumStds = Usr_GetNumUsrsInCrssOfAUsr (UsrDat->UsrCod,Rol_ROLE_STUDENT,Rol_ROLE_STUDENT);
      fprintf (Gbl.F.Out,"<td class=\"%s\""
			 " style=\"text-align:right; vertical-align:top;\">"
                         "%s"
			 "</td>"
			 "<td class=\"DAT\""
			 " style=\"text-align:left; vertical-align:top;\">"
			 "%u&nbsp;%s<br />"
			 "%u&nbsp;%s<br />"
			 "%u&nbsp;%s"
			 "</a>"
			 "</td>",
	       The_ClassFormul[Gbl.Prefs.Theme],
	       Txt_ROLES_SINGUL_Abc[Rol_ROLE_STUDENT][UsrDat->Sex],
	       NumCrssUsrIsStudent,
	       (NumCrssUsrIsStudent == 1) ? Txt_course :
	                                    Txt_courses,
	       NumTchs,
	       (NumTchs == 1) ? Txt_ROLES_SINGUL_abc[Rol_ROLE_TEACHER][Usr_SEX_UNKNOWN] :
		                Txt_ROLES_PLURAL_abc[Rol_ROLE_TEACHER][Usr_SEX_UNKNOWN],
	       NumStds,
	       (NumStds == 1) ? Txt_ROLES_SINGUL_abc[Rol_ROLE_STUDENT][Usr_SEX_UNKNOWN] :
		                Txt_ROLES_PLURAL_abc[Rol_ROLE_STUDENT][Usr_SEX_UNKNOWN]);
     }

   /***** Number of files currently published *****/
   if ((NumFiles = Brw_GetNumFilesUsr (UsrDat->UsrCod)))
      NumPublicFiles = Brw_GetNumPublicFilesUsr (UsrDat->UsrCod);
   else
      NumPublicFiles = 0;
   fprintf (Gbl.F.Out,"<td class=\"%s\""
		      " style=\"text-align:right; vertical-align:top;\">"
		      "%s"
		      "</td>"
		      "<td class=\"DAT\""
		      " style=\"text-align:left; vertical-align:top;\">"
		      "%u&nbsp;%s<br />"
		      "%u&nbsp;%s"
		      "</a>"
		      "</td>",
	    The_ClassFormul[Gbl.Prefs.Theme],
	    Txt_Files,
	    NumFiles,
	    (NumFiles == 1) ? Txt_file :
		              Txt_files,
	    NumPublicFiles,Txt_public_FILES);

   /***** End of table *****/
   fprintf (Gbl.F.Out,"</tr>"
	              "</table>");
  }

/*****************************************************************************/
/********************** Show details of user's profile ***********************/
/*****************************************************************************/

static void Prf_ShowHistoricUserProfile (const struct UsrData *UsrDat)
  {
   extern const char *The_ClassFormul[The_NUM_THEMES];
   extern const char *Txt_From_TIME;
   extern const char *Txt_day;
   extern const char *Txt_days;
   extern const char *Txt_Calculate;
   extern const char *Txt_Clicks;
   extern const char *Txt_of_PART_OF_A_TOTAL;
   extern const char *Txt_clicks;
   extern const char *Txt_Downloads;
   extern const char *Txt_download;
   extern const char *Txt_downloads;
   extern const char *Txt_Forums;
   extern const char *Txt_post;
   extern const char *Txt_posts;
   extern const char *Txt_Messages;
   extern const char *Txt_message;
   extern const char *Txt_messages;
   struct UsrFigures UsrFigures;

   /***** Start table *****/
   fprintf (Gbl.F.Out,"<table class=\"TABLE10 CELLS_PAD_2\">");

   /***** Get figures *****/
   Prf_GetUsrFigures (UsrDat->UsrCod,&UsrFigures);

   /* First click time */
   fprintf (Gbl.F.Out,"<tr>"
		      "<td class=\"%s\""
		      " style=\"text-align:right; vertical-align:top;\">"
		      "%s"
		      "</td>"
		      "<td class=\"DAT\""
		      " style=\"text-align:left; vertical-align:top;\">",
	    The_ClassFormul[Gbl.Prefs.Theme],
	    Txt_From_TIME);
   if (UsrFigures.FirstClickTime.Date.Year)
     {
      Dat_WriteDate (UsrFigures.FirstClickTime.Date.YYYYMMDD);
      if (UsrFigures.NumDays >= 0)
	 fprintf (Gbl.F.Out,"<br />"
	                    "%d&nbsp;%s",
		  UsrFigures.NumDays,
		  (UsrFigures.NumDays == 1) ? Txt_day :
					      Txt_days);
     }
   else	// First click time is unknown or user never logged
     {
      /***** Button to fetch and store first click time *****/
      Act_FormStart (ActCal1stClkTim);
      Usr_PutParamOtherUsrCodEncrypted (UsrDat->EncryptedUsrCod);
      Act_LinkFormSubmitAnimated (Txt_Calculate,The_ClassFormul[Gbl.Prefs.Theme],
                                  "calculate1","calculating1");
      Lay_PutCalculateIcon (Txt_Calculate,Txt_Calculate,
                            "calculate1","calculating1");
      Act_FormEnd ();
     }
   fprintf (Gbl.F.Out,"</td>"
		      "</tr>");

   /* Number of clicks */
   fprintf (Gbl.F.Out,"<tr>"
		      "<td class=\"%s\""
		      " style=\"text-align:right; vertical-align:top;\">"
		      "%s"
		      "</td>"
		      "<td class=\"DAT\""
		      " style=\"text-align:left; vertical-align:top;\">",
	    The_ClassFormul[Gbl.Prefs.Theme],
	    Txt_Clicks);
   if (UsrFigures.NumClicks >= 0)
     {
      fprintf (Gbl.F.Out,"%ld&nbsp;%s"
			 "<div style=\"vertical-align:middle;\">"
			 "<span class=\"RANK\">#%ld</span>"
			 "&nbsp;%s&nbsp;%ld"
	                 "</div>",
               UsrFigures.NumClicks,Txt_clicks,
               Prf_GetRankingNumClicks (UsrDat->UsrCod),
               Txt_of_PART_OF_A_TOTAL,
               Prf_GetNumUsrsWithNumClicks ());
      if (UsrFigures.NumDays >= 0)
	{
         Str_WriteFloatNum ((float) UsrFigures.NumClicks /
		            (float) (UsrFigures.NumDays + 1));
	 fprintf (Gbl.F.Out,"&nbsp;/&nbsp;%s"
	                    "<div style=\"vertical-align:middle;\">"
	                    "<span class=\"RANK\">#%ld</span>"
	                    "&nbsp;%s&nbsp;%ld"
	                    "</div>",
	          Txt_day,
		  Prf_GetRankingNumClicksPerDay (UsrDat->UsrCod),
		  Txt_of_PART_OF_A_TOTAL,
		  Prf_GetNumUsrsWithNumClicksPerDay ());
	}
     }
   else	// Number of clicks is unknown
     {
      /***** Button to fetch and store number of clicks *****/
      Act_FormStart (ActCalNumClk);
      Usr_PutParamOtherUsrCodEncrypted (UsrDat->EncryptedUsrCod);
      Act_LinkFormSubmitAnimated (Txt_Calculate,The_ClassFormul[Gbl.Prefs.Theme],
                                  "calculate2","calculating2");
      Lay_PutCalculateIcon (Txt_Calculate,Txt_Calculate,
                            "calculate2","calculating2");
      Act_FormEnd ();
     }
   fprintf (Gbl.F.Out,"</td>"
		      "</tr>");

   /***** Number of file views *****/
   fprintf (Gbl.F.Out,"<tr>"
		      "<td class=\"%s\""
		      " style=\"text-align:right; vertical-align:top;\">"
		      "%s"
		      "</td>"
		      "<td class=\"DAT\""
		      " style=\"text-align:left; vertical-align:top;\">",
	    The_ClassFormul[Gbl.Prefs.Theme],
	    Txt_Downloads);
   if (UsrFigures.NumFileViews >= 0)
     {
      fprintf (Gbl.F.Out,"%ld&nbsp;%s",
               UsrFigures.NumFileViews,
               (UsrFigures.NumFileViews == 1) ? Txt_download :
        	                                Txt_downloads);
      if (UsrFigures.NumDays >= 0)
	{
	 fprintf (Gbl.F.Out,"<br />");
         Str_WriteFloatNum ((float) UsrFigures.NumFileViews /
		            (float) (UsrFigures.NumDays + 1));
	 fprintf (Gbl.F.Out,"&nbsp;/&nbsp;%s",Txt_day);
	}
     }
   else	// Number of file views is unknown
     {
      /***** Button to fetch and store number of file views *****/
      Act_FormStart (ActCalNumFilVie);
      Usr_PutParamOtherUsrCodEncrypted (UsrDat->EncryptedUsrCod);
      Act_LinkFormSubmitAnimated (Txt_Calculate,The_ClassFormul[Gbl.Prefs.Theme],
                                  "calculate3","calculating3");
      Lay_PutCalculateIcon (Txt_Calculate,Txt_Calculate,
                            "calculate3","calculating3");
      Act_FormEnd ();
     }
   fprintf (Gbl.F.Out,"</td>"
		      "</tr>");

   /***** Number of posts in forums *****/
   fprintf (Gbl.F.Out,"<tr>"
		      "<td class=\"%s\""
		      " style=\"text-align:right; vertical-align:top;\">"
		      "%s"
		      "</td>"
		      "<td class=\"DAT\""
		      " style=\"text-align:left; vertical-align:top;\">",
	    The_ClassFormul[Gbl.Prefs.Theme],
	    Txt_Forums);
   if (UsrFigures.NumForPst >= 0)
     {
      fprintf (Gbl.F.Out,"%ld&nbsp;%s",
               UsrFigures.NumForPst,
               (UsrFigures.NumForPst == 1) ? Txt_post :
        	                             Txt_posts);
      if (UsrFigures.NumDays >= 0)
	{
	 fprintf (Gbl.F.Out,"<br />");
         Str_WriteFloatNum ((float) UsrFigures.NumForPst /
		            (float) (UsrFigures.NumDays + 1));
	 fprintf (Gbl.F.Out,"&nbsp;/&nbsp;%s",Txt_day);
	}
     }
   else	// Number of forum posts is unknown
     {
      /***** Button to fetch and store number of forum posts *****/
      Act_FormStart (ActCalNumForPst);
      Usr_PutParamOtherUsrCodEncrypted (UsrDat->EncryptedUsrCod);
      Act_LinkFormSubmitAnimated (Txt_Calculate,The_ClassFormul[Gbl.Prefs.Theme],
                                  "calculate4","calculating4");
      Lay_PutCalculateIcon (Txt_Calculate,Txt_Calculate,
                            "calculate4","calculating4");
      Act_FormEnd ();
     }
   fprintf (Gbl.F.Out,"</td>"
		      "</tr>");

   /***** Number of messages sent *****/
   fprintf (Gbl.F.Out,"<tr>"
		      "<td class=\"%s\""
		      " style=\"text-align:right; vertical-align:top;\">"
		      "%s"
		      "</td>"
		      "<td class=\"DAT\""
		      " style=\"text-align:left; vertical-align:top;\">",
	    The_ClassFormul[Gbl.Prefs.Theme],
	    Txt_Messages);
   if (UsrFigures.NumMsgSnt >= 0)
     {
      fprintf (Gbl.F.Out,"%ld&nbsp;%s",
               UsrFigures.NumMsgSnt,
               (UsrFigures.NumMsgSnt == 1) ? Txt_message :
        	                             Txt_messages);
      if (UsrFigures.NumDays >= 0)
	{
	 fprintf (Gbl.F.Out,"<br />");
         Str_WriteFloatNum ((float) UsrFigures.NumMsgSnt /
		            (float) (UsrFigures.NumDays + 1));
	 fprintf (Gbl.F.Out,"&nbsp;/&nbsp;%s",Txt_day);
	}
     }
   else	// Number of clicks is unknown
     {
      /***** Button to fetch and store number of messages sent *****/
      Act_FormStart (ActCalNumMsgSnt);
      Usr_PutParamOtherUsrCodEncrypted (UsrDat->EncryptedUsrCod);
      Act_LinkFormSubmitAnimated (Txt_Calculate,The_ClassFormul[Gbl.Prefs.Theme],
                                  "calculate5","calculating5");
      Lay_PutCalculateIcon (Txt_Calculate,Txt_Calculate,
                            "calculate5","calculating5");
      Act_FormEnd ();
     }
   fprintf (Gbl.F.Out,"</td>"
		      "</tr>");

   /***** End of table *****/
   fprintf (Gbl.F.Out,"</table>");
  }

/*****************************************************************************/
/********************** Select values on user's figures **********************/
/*****************************************************************************/

static void Prf_GetUsrFigures (long UsrCod,struct UsrFigures *UsrFigures)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumRows;

   /***** Get user's figures from database *****/
   sprintf (Query,"SELECT DATE_FORMAT(FirstClickTime,'%%Y%%m%%d%%H%%i%%S'),"
	          "DATEDIFF(NOW(),FirstClickTime),"
	          "NumClicks,NumFileViews,NumForPst,NumMsgSnt"
	          " FROM usr_figures WHERE UsrCod='%ld'",
	    UsrCod);
   if ((NumRows = (unsigned) DB_QuerySELECT (Query,&mysql_res,"can not get user's figures")))
     {
      /***** Get user's figures *****/
      row = mysql_fetch_row (mysql_res);

      /* Get first click (row[0] holds the start date in YYYYMMDDHHMMSS format) */
      if (!(Dat_GetDateTimeFromYYYYMMDDHHMMSS (&(UsrFigures->FirstClickTime),row[0])))
	 Lay_ShowErrorAndExit ("Error when reading first click time.");

      /* Get number of days since first click (row[1]) */
      if (UsrFigures->FirstClickTime.Date.Year)
	{
	 if (sscanf (row[1],"%d",&UsrFigures->NumDays) != 1)
	    UsrFigures->NumDays = -1;
	}
      else
	 UsrFigures->NumDays = -1;

      /* Get number of clicks (row[2]) */
      if (sscanf (row[2],"%ld",&UsrFigures->NumClicks) != 1)
	 UsrFigures->NumClicks = -1L;

      /* Get number of file views (row[3]) */
      if (sscanf (row[3],"%ld",&UsrFigures->NumFileViews) != 1)
	 UsrFigures->NumFileViews = -1L;

      /* Get number of forum posts (row[4]) */
      if (sscanf (row[4],"%ld",&UsrFigures->NumForPst) != 1)
	 UsrFigures->NumForPst = -1L;

      /* Get number of messages sent (row[5]) */
      if (sscanf (row[5],"%ld",&UsrFigures->NumMsgSnt) != 1)
	 UsrFigures->NumMsgSnt = -1L;
     }
   else
      /***** Return special user's figures indicating "not present" *****/
      Prf_ResetUsrFigures (UsrFigures);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********** Get ranking of a user according to the number of clicks **********/
/*****************************************************************************/

static unsigned long Prf_GetRankingNumClicks (long UsrCod)
  {
   char Query[128];

   /***** Select number of rows with number of clicks
          greater than the clicks of this user *****/
   sprintf (Query,"SELECT COUNT(*)+1 FROM usr_figures"
	          " WHERE UsrCod<>'%ld'"	// Really not necessary here
                  " AND NumClicks>"
	          "(SELECT NumClicks FROM usr_figures WHERE UsrCod='%ld')",
	    UsrCod,UsrCod);
   return DB_QueryCOUNT (Query,"can not get ranking using number of clicks");
  }

/*****************************************************************************/
/******************* Get number of users with number of clicks ***************/
/*****************************************************************************/

static unsigned long Prf_GetNumUsrsWithNumClicks (void)
  {
   char Query[128];

   /***** Select number of rows with values already calculated *****/
   sprintf (Query,"SELECT COUNT(*) FROM usr_figures WHERE NumClicks>='0'");
   return DB_QueryCOUNT (Query,"can not get number of users with number of clicks");
  }

/*****************************************************************************/
/****** Get ranking of a user according to the number of clicks per day ******/
/*****************************************************************************/

static unsigned long Prf_GetRankingNumClicksPerDay (long UsrCod)
  {
   char Query[512];

   /***** Select number of rows with number of clicks per day
          greater than the clicks per day of this user *****/
   sprintf (Query,"SELECT COUNT(*)+1 FROM"
                  " (SELECT NumClicks/(DATEDIFF(NOW(),FirstClickTime)+1)"
                  " AS NumClicksPerDay"
                  " FROM usr_figures"
                  " WHERE UsrCod<>'%ld'"	// Necessary because the following comparison is not exact in floating point
                  " AND NumClicks>='0' AND UNIX_TIMESTAMP(FirstClickTime)>'0')"
                  " AS TableNumClicksPerDay"
                  " WHERE NumClicksPerDay>"
                  "(SELECT NumClicks/(DATEDIFF(NOW(),FirstClickTime)+1)"
                  " FROM usr_figures"
                  " WHERE UsrCod='%ld'"
                  " AND NumClicks>='0' AND UNIX_TIMESTAMP(FirstClickTime)>'0')",
	    UsrCod,UsrCod);
   return DB_QueryCOUNT (Query,"can not get ranking using number of clicks per day");
  }

/*****************************************************************************/
/************** Get number of users with number of clicks per day ************/
/*****************************************************************************/

static unsigned long Prf_GetNumUsrsWithNumClicksPerDay (void)
  {
   char Query[128];

   /***** Select number of rows with values already calculated *****/
   sprintf (Query,"SELECT COUNT(*) FROM usr_figures"
	          " WHERE NumClicks>='0' AND UNIX_TIMESTAMP(FirstClickTime)>'0'");
   return DB_QueryCOUNT (Query,"can not get number of users with number of clicks per day");
  }

/*****************************************************************************/
/********* Calculate first click time and show user's profile again **********/
/*****************************************************************************/

void Prf_CalculateFirstClickTime (void)
  {
   /***** Get user's code *****/
   Usr_GetParamOtherUsrCodEncrypted ();

   /***** Get first click time from log and store as user's figure *****/
   Prf_GetFirstClickFromLogAndStoreAsUsrFigure (Gbl.Usrs.Other.UsrDat.UsrCod);

   /***** Show user's profile again *****/
   Prf_ShowUserProfile ();
  }

/*****************************************************************************/
/*** Get first click of a user from log table and store in user's figures ****/
/*****************************************************************************/

static void Prf_GetFirstClickFromLogAndStoreAsUsrFigure (long UsrCod)
  {
   char Query[256];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   struct UsrFigures UsrFigures;

   if (Usr_ChkIfUsrCodExists (UsrCod))
     {
      /***** Reset user's figures *****/
      Prf_ResetUsrFigures (&UsrFigures);

      /***** Get first click from log table *****/
      sprintf (Query,"SELECT DATE_FORMAT("
	             "(SELECT MIN(ClickTime) FROM log WHERE UsrCod='%ld')"
	             ",'%%Y%%m%%d%%H%%i%%S')",
	       UsrCod);
      if (DB_QuerySELECT (Query,&mysql_res,"can not get user's first click"))
	{
	 /* Get first click */
	 row = mysql_fetch_row (mysql_res);

	 /* Get first click (row[0] holds the start date in YYYYMMDDHHMMSS format) */
	 if (row[0])	// It is NULL when user never logged
	    if (!(Dat_GetDateTimeFromYYYYMMDDHHMMSS (&(UsrFigures.FirstClickTime),row[0])))
	       Lay_ShowErrorAndExit ("Error when reading first click time.");
	}
      /* Free structure that stores the query result */
      DB_FreeMySQLResult (&mysql_res);

      /***** Update first click time in user's figures *****/
      if (Prf_CheckIfUsrFiguresExists (UsrCod))
	{
	 sprintf (Query,"UPDATE usr_figures SET FirstClickTime='%s'"
			" WHERE UsrCod='%ld'",
		  UsrFigures.FirstClickTime.YYYYMMDDHHMMSS,UsrCod);
	 DB_QueryUPDATE (Query,"can not update user's figures");
	}
      else			// User entry does not exist
	 Prf_CreateUsrFigures (UsrCod,&UsrFigures);
     }
  }

/*****************************************************************************/
/********* Calculate number of clicks and show user's profile again **********/
/*****************************************************************************/

void Prf_CalculateNumClicks (void)
  {
   /***** Get user's code *****/
   Usr_GetParamOtherUsrCodEncrypted ();

   /***** Get number of clicks and store as user's figure *****/
   Prf_GetNumClicksAndStoreAsUsrFigure (Gbl.Usrs.Other.UsrDat.UsrCod);

   /***** Show user's profile again *****/
   Prf_ShowUserProfile ();
  }

/*****************************************************************************/
/* Get number of clicks of a user from log table and store in user's figures */
/*****************************************************************************/

static void Prf_GetNumClicksAndStoreAsUsrFigure (long UsrCod)
  {
   char Query[256];
   struct UsrFigures UsrFigures;

   if (Usr_ChkIfUsrCodExists (UsrCod))
     {
      /***** Reset user's figures *****/
      Prf_ResetUsrFigures (&UsrFigures);

      /***** Get number of clicks from database *****/
      sprintf (Query,"SELECT COUNT(*) FROM log WHERE UsrCod='%ld'",
	       UsrCod);
      UsrFigures.NumClicks = (long) DB_QueryCOUNT (Query,"can not get number of clicks");

      /***** Update number of clicks in user's figures *****/
      if (Prf_CheckIfUsrFiguresExists (UsrCod))
	{
	 sprintf (Query,"UPDATE usr_figures SET NumClicks='%ld'"
			" WHERE UsrCod='%ld'",
		  UsrFigures.NumClicks,UsrCod);
	 DB_QueryUPDATE (Query,"can not update user's figures");
	}
      else			// User entry does not exist
	 Prf_CreateUsrFigures (UsrCod,&UsrFigures);
     }
   }

/*****************************************************************************/
/******* Calculate number of file views and show user's profile again *******/
/*****************************************************************************/

void Prf_CalculateNumFileViews (void)
  {
   /***** Get user's code *****/
   Usr_GetParamOtherUsrCodEncrypted ();

   /***** Get number of file views and store as user's figure *****/
   Prf_GetNumFileViewsAndStoreAsUsrFigure (Gbl.Usrs.Other.UsrDat.UsrCod);

   /***** Show user's profile again *****/
   Prf_ShowUserProfile ();
  }

/*****************************************************************************/
/**** Get number of file views sent by a user and store in user's figures ****/
/*****************************************************************************/

static void Prf_GetNumFileViewsAndStoreAsUsrFigure (long UsrCod)
  {
   char Query[256];
   struct UsrFigures UsrFigures;

   if (Usr_ChkIfUsrCodExists (UsrCod))
     {
      /***** Reset user's figures *****/
      Prf_ResetUsrFigures (&UsrFigures);

      /***** Get number of file views from database *****/
      UsrFigures.NumFileViews = Brw_GetNumFileViewsUsr (UsrCod);

      /***** Update number of file views in user's figures *****/
      if (Prf_CheckIfUsrFiguresExists (UsrCod))
	{
	 sprintf (Query,"UPDATE usr_figures SET NumFileViews='%ld'"
			" WHERE UsrCod='%ld'",
		  UsrFigures.NumFileViews,UsrCod);
	 DB_QueryUPDATE (Query,"can not update user's figures");
	}
      else			// User entry does not exist
	 Prf_CreateUsrFigures (UsrCod,&UsrFigures);
     }
   }

/*****************************************************************************/
/******* Calculate number of forum posts and show user's profile again *******/
/*****************************************************************************/

void Prf_CalculateNumForPst (void)
  {
   /***** Get user's code *****/
   Usr_GetParamOtherUsrCodEncrypted ();

   /***** Get number of forum posts and store as user's figure *****/
   Prf_GetNumForPstAndStoreAsUsrFigure (Gbl.Usrs.Other.UsrDat.UsrCod);

   /***** Show user's profile again *****/
   Prf_ShowUserProfile ();
  }

/*****************************************************************************/
/**** Get number of forum posts sent by a user and store in user's figures ***/
/*****************************************************************************/

static void Prf_GetNumForPstAndStoreAsUsrFigure (long UsrCod)
  {
   char Query[256];
   struct UsrFigures UsrFigures;

   if (Usr_ChkIfUsrCodExists (UsrCod))
     {
      /***** Reset user's figures *****/
      Prf_ResetUsrFigures (&UsrFigures);

      /***** Get number of forum posts from database *****/
      UsrFigures.NumForPst = For_GetNumPostsUsr (UsrCod);

      /***** Update number of forum posts in user's figures *****/
      if (Prf_CheckIfUsrFiguresExists (UsrCod))
	{
	 sprintf (Query,"UPDATE usr_figures SET NumForPst='%ld'"
			" WHERE UsrCod='%ld'",
		  UsrFigures.NumForPst,UsrCod);
	 DB_QueryUPDATE (Query,"can not update user's figures");
	}
      else			// User entry does not exist
	 Prf_CreateUsrFigures (UsrCod,&UsrFigures);
     }
   }

/*****************************************************************************/
/****** Calculate number of messages sent and show user's profile again ******/
/*****************************************************************************/

void Prf_CalculateNumMsgSnt (void)
  {
   /***** Get user's code *****/
   Usr_GetParamOtherUsrCodEncrypted ();

   /***** Get number of messages sent and store as user's figure *****/
   Prf_GetNumMsgSntAndStoreAsUsrFigure (Gbl.Usrs.Other.UsrDat.UsrCod);

   /***** Show user's profile again *****/
   Prf_ShowUserProfile ();
  }

/*****************************************************************************/
/***** Get number of messages sent by a user and store in user's figures *****/
/*****************************************************************************/

static void Prf_GetNumMsgSntAndStoreAsUsrFigure (long UsrCod)
  {
   char Query[256];
   struct UsrFigures UsrFigures;

   if (Usr_ChkIfUsrCodExists (UsrCod))
     {
      /***** Reset user's figures *****/
      Prf_ResetUsrFigures (&UsrFigures);

      /***** Get number of messages sent from database *****/
      UsrFigures.NumMsgSnt = Msg_GetNumMsgsSentByUsr (UsrCod);

      /***** Update number of messages sent in user's figures *****/
      if (Prf_CheckIfUsrFiguresExists (UsrCod))
	{
	 sprintf (Query,"UPDATE usr_figures SET NumMsgSnt='%ld'"
			" WHERE UsrCod='%ld'",
		  UsrFigures.NumMsgSnt,UsrCod);
	 DB_QueryUPDATE (Query,"can not update user's figures");
	}
      else			// User entry does not exist
	 Prf_CreateUsrFigures (UsrCod,&UsrFigures);
     }
  }

/*****************************************************************************/
/********************** Reset values of user's figures ***********************/
/*****************************************************************************/

static void Prf_ResetUsrFigures (struct UsrFigures *UsrFigures)
  {
   Dat_GetDateTimeFromYYYYMMDDHHMMSS (&(UsrFigures->FirstClickTime),"00000000000000");	//  unknown first click time or user never logged
   UsrFigures->NumDays      = -1;		// not applicable
   UsrFigures->NumClicks    = -1L;		// unknown number of clicks
   UsrFigures->NumFileViews = -1L;		// unknown number of file views
   UsrFigures->NumForPst    = -1L;		// unknown number of forum posts
   UsrFigures->NumMsgSnt    = -1L;		// unknown number of messages sent
  }

/*****************************************************************************/
/***** Get number of messages sent by a user and store in user's figures *****/
/*****************************************************************************/

static void Prf_CreateUsrFigures (long UsrCod,const struct UsrFigures *UsrFigures)
  {
   char Query[256];

   /***** Create user's figures *****/
   sprintf (Query,"INSERT INTO usr_figures (UsrCod,FirstClickTime,NumClicks,NumFileViews,NumForPst,NumMsgSnt)"
		  " VALUES ('%ld','%s','%ld','%ld','%ld','%ld')",
	    UsrCod,
	    UsrFigures->FirstClickTime.YYYYMMDDHHMMSS,	//   0 ==> unknown first click time or user never logged
	    UsrFigures->NumClicks,			// -1L ==> unknown number of clicks
	    UsrFigures->NumFileViews,			// -1L ==> unknown number of file views
	    UsrFigures->NumForPst,			// -1L ==> unknown number of forum posts
	    UsrFigures->NumMsgSnt);			// -1L ==> unknown number of messages sent
   DB_QueryINSERT (Query,"can not create user's figures");
  }

/*****************************************************************************/
/**************************** Remove user's figures **************************/
/*****************************************************************************/

void Prf_RemoveUsrFigures (long UsrCod)
  {
   char Query[128];

   /***** Remove user's figures *****/
   sprintf (Query,"DELETE FROM usr_figures WHERE UsrCod='%ld'",
	    UsrCod);
   DB_QueryDELETE (Query,"can not delete user's figures");
  }

/*****************************************************************************/
/*** Check if it exists an entry for this user in table of user's figures ****/
/*****************************************************************************/

static bool Prf_CheckIfUsrFiguresExists (long UsrCod)
  {
   char Query[128];

   sprintf (Query,"SELECT COUNT(*) FROM usr_figures WHERE UsrCod='%ld'",
	    UsrCod);
   return (DB_QueryCOUNT (Query,"can not get user's first click") != 0);
  }

/*****************************************************************************/
/*************** Increment number of clicks made by a user *******************/
/*****************************************************************************/

void Prf_IncrementNumClicksUsr (long UsrCod)
  {
   char Query[256];

   /***** Increment number of clicks *****/
   // If NumClicks < 0 ==> not yet calculated, so do nothing
   sprintf (Query,"UPDATE IGNORE usr_figures SET NumClicks=NumClicks+1"
	          " WHERE UsrCod='%ld' AND NumClicks>=0",
	    UsrCod);
   DB_QueryINSERT (Query,"can not increment user's clicks");
  }

/*****************************************************************************/
/************** Increment number of file views sent by a user ****************/
/*****************************************************************************/

void Prf_IncrementNumFileViewsUsr (long UsrCod)
  {
   char Query[256];

   /***** Increment number of file views *****/
   // If NumFileViews < 0 ==> not yet calculated, so do nothing
   sprintf (Query,"UPDATE IGNORE usr_figures SET NumFileViews=NumFileViews+1"
	          " WHERE UsrCod='%ld' AND NumFileViews>=0",
	    UsrCod);
   DB_QueryINSERT (Query,"can not increment user's file views");
  }

/*****************************************************************************/
/************* Increment number of forum posts sent by a user ****************/
/*****************************************************************************/

void Prf_IncrementNumForPstUsr (long UsrCod)
  {
   char Query[256];

   /***** Increment number of forum posts *****/
   // If NumForPst < 0 ==> not yet calculated, so do nothing
   sprintf (Query,"UPDATE IGNORE usr_figures SET NumForPst=NumForPst+1"
	          " WHERE UsrCod='%ld' AND NumForPst>=0",
	    UsrCod);
   DB_QueryINSERT (Query,"can not increment user's forum posts");
  }

/*****************************************************************************/
/*************** Increment number of messages sent by a user *****************/
/*****************************************************************************/

void Prf_IncrementNumMsgSntUsr (long UsrCod)
  {
   char Query[256];

   /***** Increment number of messages sent *****/
   // If NumMsgSnt < 0 ==> not yet calculated, so do nothing
   sprintf (Query,"UPDATE IGNORE usr_figures SET NumMsgSnt=NumMsgSnt+1"
	          " WHERE UsrCod='%ld' AND NumMsgSnt>=0",
	    UsrCod);
   DB_QueryINSERT (Query,"can not increment user's messages sent");
  }
