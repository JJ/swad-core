// swad_test.c: self-assessment tests

/*
    SWAD (Shared Workspace At a Distance),
    is a web platform developed at the University of Granada (Spain),
    and used to support university teaching.

    This file is part of SWAD core.
    Copyright (C) 1999-2016 Antonio Ca�as Vargas

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
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

#include <limits.h>		// For UINT_MAX
#include <linux/limits.h>	// For PATH_MAX
#include <linux/stddef.h>	// For NULL
#include <locale.h>		// For setlocale, LC_NUMERIC...
#include <mysql/mysql.h>	// To access MySQL databases
#include <stdbool.h>		// For boolean type
#include <stdio.h>		// For fprintf, etc.
#include <stdlib.h>		// For exit, system, malloc, free, etc
#include <string.h>		// For string functions
#include <sys/stat.h>		// For mkdir
#include <sys/types.h>		// For mkdir

#include "swad_action.h"
#include "swad_database.h"
#include "swad_global.h"
#include "swad_ID.h"
#include "swad_image.h"
#include "swad_parameter.h"
#include "swad_theme.h"
#include "swad_test.h"
#include "swad_test_import.h"
#include "swad_text.h"
#include "swad_user.h"
#include "swad_xml.h"

/*****************************************************************************/
/***************************** Public constants ******************************/
/*****************************************************************************/

const char *Tst_FeedbackXML[Tst_NUM_TYPES_FEEDBACK] =
  {
   "nothing",
   "totalResult",
   "eachResult",
   "eachGoodBad",
   "fullFeedback",
  };
const char *Tst_StrAnswerTypesXML[Tst_NUM_ANS_TYPES] =
  {
   "int",
   "float",
   "TF",
   "uniqueChoice",
   "multipleChoice",
   "text",
  };

/*****************************************************************************/
/**************************** Private constants ******************************/
/*****************************************************************************/

#define Tst_MAX_BYTES_TAGS_LIST		(16*1024)
#define Tst_MAX_BYTES_FLOAT_ANSWER	30	// Maximum length of the strings that store an floating point answer

const char *Tst_PluggableDB[Tst_NUM_OPTIONS_PLUGGABLE] =
  {
   "unknown",
   "N",
   "Y",
  };

// Feedback to students in exams
const char *Tst_FeedbackDB[Tst_NUM_TYPES_FEEDBACK] =
  {
   "nothing",		// No feedback
   "total_result",	// Little
   "each_result",	// Medium
   "each_good_bad",	// High
   "full_feedback",	// Maximum
  };

const char *Tst_StrAnswerTypesDB[Tst_NUM_ANS_TYPES] =
  {
   "int",
   "float",
   "true_false",
   "unique_choice",
   "multiple_choice",
   "text",
  };

// Test photo will be saved with:
// - maximum width of Tst_PHOTO_SAVED_MAX_HEIGHT
// - maximum height of Tst_PHOTO_SAVED_MAX_HEIGHT
// - maintaining the original aspect ratio (aspect ratio recommended: 3:2)
#define Tst_PHOTO_SAVED_MAX_WIDTH	768
#define Tst_PHOTO_SAVED_MAX_HEIGHT	512
#define Tst_PHOTO_SAVED_QUALITY		 75	// 1 to 100

/*****************************************************************************/
/******************************* Internal types ******************************/
/*****************************************************************************/

#define Tst_NUM_STATUS 2
typedef enum
  {
   Tst_STATUS_SHOWN_BUT_NOT_ASSESSED	= 0,
   Tst_STATUS_ASSESSED			= 1,
   Tst_STATUS_ERROR			= 2,
  } Tst_Status_t;

/*****************************************************************************/
/************** External global variables from others modules ****************/
/*****************************************************************************/

extern struct Globals Gbl;

/*****************************************************************************/
/************************* Internal global variables *************************/
/*****************************************************************************/

/*****************************************************************************/
/***************************** Internal prototypes ***************************/
/*****************************************************************************/

static void Tst_PutFormToSeeResultsOfUsersTests (void);
static void Tst_PutIconToEdit (void);
static void Tst_PutFormToConfigure (void);

static void Tst_GetQuestionsAndAnswersFromForm (void);
static void Tst_ShowTstTotalMark (double TotalScore);
static bool Tst_CheckIfNextTstAllowed (void);
static void Tst_SetTstStatus (unsigned NumTst,Tst_Status_t TstStatus);
static Tst_Status_t Tst_GetTstStatus (unsigned NumTst);
static unsigned Tst_GetNumAccessesTst (void);
static void Tst_ShowTestQuestionsWhenSeeing (MYSQL_RES *mysql_res);
static void Tst_ShowTstResultAfterAssess (long TstCod,unsigned *NumQstsNotBlank,double *TotalScore);
static void Tst_WriteQstAndAnsExam (unsigned NumQst,long QstCod,MYSQL_ROW row,
                                    double *ScoreThisQst,bool *AnswerIsNotBlank);
static void Tst_PutFormToEditQstImage (struct Image *Image,
                                       const char *ClassImg,const char *ClassTitle,
                                       const char *ParamAction,
                                       const char *ParamFile,
                                       const char *ParamTitle,
                                       bool OptionsDisabled);
static void Tst_UpdateScoreQst (long QstCod,float ScoreThisQst,bool AnswerIsNotBlank);
static void Tst_UpdateMyNumAccessTst (unsigned NumAccessesTst);
static void Tst_UpdateLastAccTst (void);
static void Tst_PutIconToCreateNewTstQst (void);
static void Tst_PutButtonToAddQuestion (void);
static long Tst_GetParamTagCode (void);
static bool Tst_CheckIfCurrentCrsHasTestTags (void);
static unsigned long Tst_GetAllTagsFromCurrentCrs (MYSQL_RES **mysql_res);
static unsigned long Tst_GetEnabledTagsFromThisCrs (MYSQL_RES **mysql_res);
static void Tst_ShowFormSelTags (unsigned long NumRows,MYSQL_RES *mysql_res,
                                 bool ShowOnlyEnabledTags,unsigned NumCols);
static void Tst_ShowFormEditTags (void);
static void Tst_PutIconEnable (long TagCod,const char *TagTxt);
static void Tst_PutIconDisable (long TagCod,const char *TagTxt);
static void Tst_ShowFormConfigTst (void);
static void Tst_GetConfigTstFromDB (void);
static Tst_Pluggable_t Tst_GetPluggableFromForm (void);
static Tst_Feedback_t Tst_GetFeedbackTypeFromForm (void);
static void Tst_CheckAndCorrectNumbersQst (void);
static void Tst_ShowFormAnswerTypes (unsigned NumCols);
static unsigned long Tst_GetQuestionsForEdit (MYSQL_RES **mysql_res);
static unsigned long Tst_GetQuestionsForExam (MYSQL_RES **mysql_res);
static void Tst_ListOneQstToEdit (void);
static bool Tst_GetOneQuestionByCod (long QstCod,MYSQL_RES **mysql_res);
static void Tst_ListOneOrMoreQuestionsToEdit (unsigned long NumRows,MYSQL_RES *mysql_res);

static void Tst_WriteAnswersOfAQstEdit (long QstCod);
static void Tst_WriteAnswersOfAQstSeeExam (unsigned NumQst,long QstCod,bool Shuffle);
static void Tst_WriteAnswersOfAQstAssessExam (unsigned NumQst,long QstCod,
                                              double *ScoreThisQst,bool *AnswerIsNotBlank);
static void Tst_WriteFormAnsTF (unsigned NumQst);
static void Tst_WriteTFAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                      double *ScoreThisQst,bool *AnswerIsNotBlank);
static void Tst_WriteChoiceAnsSeeExam (unsigned NumQst,long QstCod,bool Shuffle);
static void Tst_WriteChoiceAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                          double *ScoreThisQst,bool *AnswerIsNotBlank);
static void Tst_WriteTextAnsSeeExam (unsigned NumQst);
static void Tst_WriteTextAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                        double *ScoreThisQst,bool *AnswerIsNotBlank);
static void Tst_WriteIntAnsSeeExam (unsigned NumQst);
static void Tst_WriteIntAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                       double *ScoreThisQst,bool *AnswerIsNotBlank);
static void Tst_WriteFloatAnsSeeExam (unsigned NumQst);
static void Tst_WriteFloatAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                         double *ScoreThisQst,bool *AnswerIsNotBlank);
static void Tst_WriteHeadUserCorrect (void);
static void Tst_WriteScoreStart (unsigned ColSpan);
static void Tst_WriteScoreEnd (void);
static void Tst_WriteParamQstCod (unsigned NumQst,long QstCod);
static void Tst_GetAndWriteTagsQst (long QstCod);
static bool Tst_GetParamsTst (void);
static unsigned Tst_GetAndCheckParamNumTst (void);
static void Tst_GetParamNumQst (void);
static bool Tst_GetCreateXMLFromForm (void);
static int Tst_CountNumTagsInList (void);
static int Tst_CountNumAnswerTypesInList (void);
static void Tst_PutFormEditOneQst (char *Stem,char *Feedback);

static void Tst_FreeTextChoiceAnswers (void);
static void Tst_FreeTextChoiceAnswer (unsigned NumOpt);

static void Tst_InitImagesOfQuestion (void);
static void Tst_FreeImagesOfQuestion (void);

static void Tst_GetQstDataFromDB (char *Stem,char *Feedback);
static void Tst_GetImageFromDB (unsigned NumOpt,struct Image *Image);

static Tst_AnswerType_t Tst_ConvertFromUnsignedStrToAnsTyp (const char *UnsignedStr);
static void Tst_GetQstFromForm (char *Stem,char *Feedback);
static void Tst_MoveImagesToDefinitiveDirectories (void);

static long Tst_GetTagCodFromTagTxt (const char *TagTxt);
static long Tst_CreateNewTag (long CrsCod,const char *TagTxt);
static void Tst_EnableOrDisableTag (long TagCod,bool TagHidden);

static void Tst_PutIconToRemoveOneQst (void);
static void Tst_PutParamsRemoveOneQst (void);

static long Tst_GetQstCod (void);

static void Tst_InsertOrUpdateQstIntoDB (void);
static void Tst_InsertTagsIntoDB (void);
static void Tst_InsertAnswersIntoDB (void);

static void Tst_RemAnsFromQst (void);
static void Tst_RemTagsFromQst (void);
static void Tst_RemoveUnusedTagsFromCurrentCrs (void);

static void Tst_RemoveImgFileFromStemOfQst (long CrsCod,long QstCod);
static void Tst_RemoveAllImgFilesFromStemOfAllQstsInCrs (long CrsCod);
static void Tst_RemoveImgFileFromAnsOfQst (long CrsCod,long QstCod,unsigned AnsInd);
static void Tst_RemoveAllImgFilesFromAnsOfQst (long CrsCod,long QstCod);
static void Tst_RemoveAllImgFilesFromAnsOfAllQstsInCrs (long CrsCod);

static unsigned Tst_GetNumTstQuestions (Sco_Scope_t Scope,Tst_AnswerType_t AnsType,struct Tst_Stats *Stats);
static unsigned Tst_GetNumCoursesWithTstQuestions (Sco_Scope_t Scope,Tst_AnswerType_t AnsType);
static unsigned Tst_GetNumCoursesWithPluggableTstQuestions (Sco_Scope_t Scope,Tst_AnswerType_t AnsType);

static long Tst_CreateTestExamInDB (void);
static void Tst_StoreScoreOfTestExamInDB (long TstCod,
                                          unsigned NumQstsNotBlank,double Score);
static void Tst_ShowHeaderTestResults (void);
static void Tst_ShowResultsOfTestExams (struct UsrData *UsrDat);
static void Tst_ShowDataUsr (struct UsrData *UsrDat,unsigned NumExams);
static void Tst_PutParamTstCod (long TstCod);
static long Tst_GetParamTstCod (void);
static void Tst_ShowExamTstResult (time_t TstTimeUTC);
static void Tst_GetExamDataByTstCod (long TstCod,time_t *TstTimeUTC,
                                     unsigned *NumQsts,unsigned *NumQstsNotBlank,double *Score);
static void Tst_StoreOneExamQstInDB (long TstCod,long QstCod,unsigned NumQst,double Score);
static void Tst_GetExamQuestionsFromDB (long TstCod);

/*****************************************************************************/
/*************** Show form to generate a self-assessment test ****************/
/*****************************************************************************/

void Tst_ShowFormAskTst (void)
  {
   extern const char *The_ClassForm[The_NUM_THEMES];
   extern const char *Txt_Test;
   extern const char *Txt_No_of_questions;
   extern const char *Txt_Generate_exam;
   extern const char *Txt_No_test_questions;
   MYSQL_RES *mysql_res;
   unsigned long NumRows;
   bool ICanEdit = (Gbl.Usrs.Me.LoggedRole == Rol_STUDENT ||
                    Gbl.Usrs.Me.LoggedRole == Rol_TEACHER ||
                    Gbl.Usrs.Me.LoggedRole == Rol_SYS_ADM);

   /***** Read test configuration from database *****/
   Tst_GetConfigTstFromDB ();

   /***** Put form to go to test edition and configuration *****/
   if (ICanEdit)
     {
      fprintf (Gbl.F.Out,"<div class=\"CONTEXT_MENU\">");
      Tst_PutFormToSeeResultsOfUsersTests ();
      Tst_PutFormToConfigure ();
      fprintf (Gbl.F.Out,"</div>");
     }

   /***** Start frame *****/
   Lay_StartRoundFrame (NULL,Txt_Test,ICanEdit ? Tst_PutIconToEdit :
	                                         NULL);

   /***** Get tags *****/
   if ((NumRows = Tst_GetEnabledTagsFromThisCrs (&mysql_res)) != 0)
     {
      /***** Check if minimum date-time of next access to test is older than now *****/
      if (Tst_CheckIfNextTstAllowed ())
        {
         Act_FormStart (ActSeeTst);
         fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">");

         /***** Selection of tags *****/
         Tst_ShowFormSelTags (NumRows,mysql_res,true,1);

         /***** Selection of types of answers *****/
         Tst_ShowFormAnswerTypes (1);

         /***** Number of questions to generate ****/
         fprintf (Gbl.F.Out,"<tr>"
                            "<td class=\"RIGHT_MIDDLE\">"
                            "<label class=\"%s\">"
                            "%s:"
                            "</label>"
                            "</td>"
                            "<td class=\"LEFT_MIDDLE\">"
                            "<input type=\"text\" name=\"NumQst\""
                            " size=\"3\" maxlength=\"3\" value=\"%u\"",
                  The_ClassForm[Gbl.Prefs.Theme],Txt_No_of_questions,
                  Gbl.Test.Config.Def);
         if (Gbl.Test.Config.Min == Gbl.Test.Config.Max)
            fprintf (Gbl.F.Out," disabled=\"disabled\"");
         fprintf (Gbl.F.Out," />"
                            "</td>"
                            "</tr>"
                            "</table>");

         /***** Send button *****/
         Lay_PutConfirmButton (Txt_Generate_exam);
         Act_FormEnd ();
        }
     }
   else
     {
      /***** Warning message *****/
      Lay_ShowAlert (Lay_INFO,Txt_No_test_questions);

      /***** Button to create a new question *****/
      Tst_PutButtonToAddQuestion ();
     }

   /***** End frame *****/
   Lay_EndRoundFrame ();

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*************** Write a form to go to result of users' tests ****************/
/*****************************************************************************/

static void Tst_PutFormToSeeResultsOfUsersTests (void)
  {
   extern const char *Txt_Results_tests;

   Lay_PutContextualLink (Gbl.Usrs.Me.LoggedRole == Rol_STUDENT ? ActReqSeeMyTstExa :
	                                                               ActReqSeeUsrTstExa,
	                  NULL,"file64x64.gif",
	                  Txt_Results_tests,Txt_Results_tests);
  }

/*****************************************************************************/
/*************** Write icon to go to edition of test questions ***************/
/*****************************************************************************/

static void Tst_PutIconToEdit (void)
  {
   extern const char *Txt_Edit;

   Lay_PutContextualLink (ActEdiTstQst,NULL,"edit64x64.png",Txt_Edit,NULL);
  }

/*****************************************************************************/
/************** Write a form to go to configuration of tests *****************/
/*****************************************************************************/

static void Tst_PutFormToConfigure (void)
  {
   extern const char *Txt_Configure;

   Lay_PutContextualLink (ActCfgTst,NULL,"config64x64.gif",
                          Txt_Configure,Txt_Configure);
  }

/*****************************************************************************/
/********************** Generate self-assessment test ************************/
/*****************************************************************************/

void Tst_ShowNewTestExam (void)
  {
   extern const char *The_ClassForm[The_NUM_THEMES];
   extern const char *Txt_No_questions_found_matching_your_search_criteria;
   extern const char *Txt_Test;
   extern const char *Txt_Allow_teachers_to_consult_this_exam;
   extern const char *Txt_Done_assess_exam;
   MYSQL_RES *mysql_res;
   unsigned long NumRows;
   unsigned NumAccessesTst;

   /***** Read test configuration from database *****/
   Tst_GetConfigTstFromDB ();

   if (Tst_CheckIfNextTstAllowed ())
     {
      /***** Check that all parameters used to generate a test are valid *****/
      if (Tst_GetParamsTst ())					// Get parameters from form
        {
         /***** Get questions *****/
         if ((NumRows = Tst_GetQuestionsForExam (&mysql_res)) == 0)	// Query database
           {
            Lay_ShowAlert (Lay_INFO,Txt_No_questions_found_matching_your_search_criteria);
            Tst_ShowFormAskTst ();					// Show the form again
           }
         else
           {
            /***** Get and update number of hits *****/
            NumAccessesTst = Tst_GetNumAccessesTst () + 1;
            if (Gbl.Usrs.Me.IBelongToCurrentCrs)
	       Tst_UpdateMyNumAccessTst (NumAccessesTst);

	    /***** Start frame *****/
	    Lay_StartRoundFrame (NULL,Txt_Test,NULL);
	    Lay_WriteHeaderClassPhoto (false,false,
				       Gbl.CurrentIns.Ins.InsCod,
				       Gbl.CurrentDeg.Deg.DegCod,
				       Gbl.CurrentCrs.Crs.CrsCod);

            /***** Start form *****/
            Act_FormStart (ActAssTst);
  	    Gbl.Test.NumQsts = (unsigned) NumRows;
            Par_PutHiddenParamUnsigned ("NumTst",NumAccessesTst);
            Par_PutHiddenParamUnsigned ("NumQst",Gbl.Test.NumQsts);

            /***** List the questions *****/
	    fprintf (Gbl.F.Out,"<table class=\"FRAME_TABLE CELLS_PAD_10\">");
            Tst_ShowTestQuestionsWhenSeeing (mysql_res);
	    fprintf (Gbl.F.Out,"</table>");

	    /***** Exam will be saved? *****/
	    fprintf (Gbl.F.Out,"<div class=\"%s CENTER_MIDDLE\""
		               " style=\"margin-top:20px;\">"
			       "<input type=\"checkbox\" name=\"Save\" value=\"Y\"",
		     The_ClassForm[Gbl.Prefs.Theme]);
	    if (Gbl.Test.AllowTeachers)
	       fprintf (Gbl.F.Out," checked=\"checked\"");
	    fprintf (Gbl.F.Out," />%s"
			       "</div>",
		     Txt_Allow_teachers_to_consult_this_exam);

            /***** End form *****/
            Lay_PutConfirmButton (Txt_Done_assess_exam);
            Act_FormEnd ();

            /***** End frame *****/
	    Lay_EndRoundFrame ();

            /***** Set test status *****/
            Tst_SetTstStatus (NumAccessesTst,Tst_STATUS_SHOWN_BUT_NOT_ASSESSED);

            /***** Update date-time of my next allowed access to test *****/
            if (Gbl.Usrs.Me.LoggedRole == Rol_STUDENT)
               Tst_UpdateLastAccTst ();
           }

         /***** Free structure that stores the query result *****/
         DB_FreeMySQLResult (&mysql_res);
        }
      else
         Tst_ShowFormAskTst ();							// Show the form again

      /***** Free memory used for by the list of tags *****/
      Tst_FreeTagsList ();
     }
  }

/*****************************************************************************/
/****************************** Assess a test exam ***************************/
/*****************************************************************************/

void Tst_AssessTestExam (void)
  {
   extern const char *Txt_Test_result;
   extern const char *Txt_Test_No_X_that_you_make_in_this_course;
   extern const char *Txt_The_test_X_has_already_been_assessed_previously;
   extern const char *Txt_There_was_an_error_in_assessing_the_test_X;
   unsigned NumTst;
   char YN[1+1];
   long TstCod = -1L;	// Initialized to avoid warning
   unsigned NumQstsNotBlank;
   double TotalScore;

   /***** Read test configuration from database *****/
   Tst_GetConfigTstFromDB ();

   /***** Get number of this test from form *****/
   NumTst = Tst_GetAndCheckParamNumTst ();

   /****** Get test status in database for this session-course-num.test *****/
   switch (Tst_GetTstStatus (NumTst))
     {
      case Tst_STATUS_SHOWN_BUT_NOT_ASSESSED:
         /***** Get the parameters of the form *****/
         /* Get number of questions */
         Tst_GetParamNumQst ();

         /***** Get if exam must be saved *****/
	 Par_GetParToText ("Save",YN,1);
	 Gbl.Test.AllowTeachers = (Str_ConvertToUpperLetter (YN[0]) == 'Y');

	 /***** Get questions and answers from form to assess an exam *****/
	 Tst_GetQuestionsAndAnswersFromForm ();

	 /***** Create new test exam in database to store the result *****/
	 TstCod = Tst_CreateTestExamInDB ();

	 /***** Start frame *****/
	 Lay_StartRoundFrame (NULL,Txt_Test_result,NULL);
	 Lay_WriteHeaderClassPhoto (false,false,
				    Gbl.CurrentIns.Ins.InsCod,
				    Gbl.CurrentDeg.Deg.DegCod,
				    Gbl.CurrentCrs.Crs.CrsCod);

	 /***** Header *****/
	 if (Gbl.Usrs.Me.IBelongToCurrentCrs)
	   {
	    fprintf (Gbl.F.Out,"<div class=\"TEST_SUBTITLE\">");
	    fprintf (Gbl.F.Out,Txt_Test_No_X_that_you_make_in_this_course,NumTst);
	    fprintf (Gbl.F.Out,"</div>");
	   }

	 /***** Write answers and solutions *****/
	 fprintf (Gbl.F.Out,"<table class=\"FRAME_TABLE CELLS_PAD_10\">");
	 Tst_ShowTstResultAfterAssess (TstCod,&NumQstsNotBlank,&TotalScore);
	 fprintf (Gbl.F.Out,"</table>");

	 /***** Write total mark of test *****/
	 if (Gbl.Test.Config.FeedbackType != Tst_FEEDBACK_NOTHING)
	    Tst_ShowTstTotalMark (TotalScore);

	 /***** End frame *****/
	 Lay_EndRoundFrame ();

	 /***** Store test result in database *****/
	 Tst_StoreScoreOfTestExamInDB (TstCod,
				       NumQstsNotBlank,TotalScore);

         /***** Set test status *****/
         Tst_SetTstStatus (NumTst,Tst_STATUS_ASSESSED);
         break;
      case Tst_STATUS_ASSESSED:
         sprintf (Gbl.Message,Txt_The_test_X_has_already_been_assessed_previously,
                  NumTst);
         Lay_ShowAlert (Lay_WARNING,Gbl.Message);
         break;
      case Tst_STATUS_ERROR:
         sprintf (Gbl.Message,Txt_There_was_an_error_in_assessing_the_test_X,
                  NumTst);
         Lay_ShowAlert (Lay_WARNING,Gbl.Message);
         break;
     }
  }

/*****************************************************************************/
/********** Get questions and answers from form to assess an exam ************/
/*****************************************************************************/

static void Tst_GetQuestionsAndAnswersFromForm (void)
  {
   unsigned NumQst;
   char StrQstIndOrAns[3+10+1];	// "Qstxx...x", "Indxx...x" or "Ansxx...x"
   char LongStr[1+10+1];

   /***** Get questions and answers *****/
   for (NumQst = 0;
	NumQst < Gbl.Test.NumQsts;
	NumQst++)
     {
      /* Get question code */
      sprintf (StrQstIndOrAns,"Qst%06u",NumQst);
      Par_GetParToText (StrQstIndOrAns,LongStr,1+10);
      if ((Gbl.Test.QstCodes[NumQst] = Str_ConvertStrCodToLongCod (LongStr)) < 0)
	 Lay_ShowErrorAndExit ("Code of question is missing.");

      /* Get indexes for this question */
      sprintf (StrQstIndOrAns,"Ind%06u",NumQst);
      Par_GetParMultiToText (StrQstIndOrAns,Gbl.Test.StrIndexesOneQst[NumQst],Tst_MAX_SIZE_INDEXES_ONE_QST);  /* If choice ==> "0", "1", "2",... */

      /* Get answers selected by user for this question */
      sprintf (StrQstIndOrAns,"Ans%06u",NumQst);
      Par_GetParMultiToText (StrQstIndOrAns,Gbl.Test.StrAnswersOneQst[NumQst],Tst_MAX_SIZE_ANSWERS_ONE_QST);  /* If answer type == T/F ==> " ", "T", "F"; if choice ==> "0", "2",... */
     }
  }

/*****************************************************************************/
/************************ Show total mark of a test exam *********************/
/*****************************************************************************/

static void Tst_ShowTstTotalMark (double TotalScore)
  {
   extern const char *Txt_Score;
   extern const char *Txt_out_of_PART_OF_A_SCORE;
   double TotalScoreOverSCORE_MAX = TotalScore * Tst_SCORE_MAX / (double) Gbl.Test.NumQsts;

   /***** Write total mark ****/
   fprintf (Gbl.F.Out,"<div class=\"DAT CENTER_MIDDLE\""
	              " style=\"margin-top:20px;\">"
		      "%s: <span class=\"%s\">%.2lf (%.2lf %s %u)</span>"
		      "</div>",
	    Txt_Score,
	    (TotalScoreOverSCORE_MAX >= (double) TotalScoreOverSCORE_MAX / 2.0) ? "ANS_OK" :
					                                          "ANS_BAD",
	    TotalScore,
	    TotalScoreOverSCORE_MAX,Txt_out_of_PART_OF_A_SCORE,Tst_SCORE_MAX);
  }

/*****************************************************************************/
/************** Check minimum date-time of next access to test ***************/
/*****************************************************************************/
// Return true if allowed date-time of next access to test is older than now

static bool Tst_CheckIfNextTstAllowed (void)
  {
   extern const char *Txt_Test;
   extern const char *Txt_You_can_not_make_a_new_test_in_the_course_X_until;
   extern const char *Txt_Today;
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   long NumSecondsFromNowToNextAccTst = -1L;	// Access allowed when this number <= 0
   time_t TimeNextTestUTC = (time_t) 0;

   /***** Superusers are allowed to do all test they want *****/
   if (Gbl.Usrs.Me.LoggedRole == Rol_SYS_ADM)
      return true;

   /***** Get date of next allowed access to test from database *****/
   sprintf (Query,"SELECT UNIX_TIMESTAMP(LastAccTst+INTERVAL (NumQstsLastTst*%lu) SECOND)-UNIX_TIMESTAMP(),"
	          "UNIX_TIMESTAMP(LastAccTst+INTERVAL (NumQstsLastTst*%lu) SECOND)"
                  " FROM crs_usr"
                  " WHERE CrsCod='%ld' AND UsrCod='%ld'",
            Gbl.Test.Config.MinTimeNxtTstPerQst,
            Gbl.Test.Config.MinTimeNxtTstPerQst,
            Gbl.CurrentCrs.Crs.CrsCod,Gbl.Usrs.Me.UsrDat.UsrCod);
   if (DB_QuerySELECT (Query,&mysql_res,"can not get last access to test") == 1)
     {
      /* Get seconds from now to next access to test */
      row = mysql_fetch_row (mysql_res);
      if (row[0])
         if (sscanf (row[0],"%ld",&NumSecondsFromNowToNextAccTst) == 1)
            /* Time UTC of next access allowed (row[1]) */
            TimeNextTestUTC = Dat_GetUNIXTimeFromStr (row[1]);
     }
   else
      Lay_ShowErrorAndExit ("Error when reading date of next allowed access to test.");

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Check if access is allowed *****/
   if (NumSecondsFromNowToNextAccTst > 0)
     {
      /***** Start frame *****/
      Lay_StartRoundFrame (NULL,Txt_Test,NULL);
      Lay_WriteHeaderClassPhoto (false,false,
				 Gbl.CurrentIns.Ins.InsCod,
				 Gbl.CurrentDeg.Deg.DegCod,
				 Gbl.CurrentCrs.Crs.CrsCod);

      /***** Write warning *****/
      fprintf (Gbl.F.Out,"<div class=\"DAT CENTER_MIDDLE\">");
      fprintf (Gbl.F.Out,Txt_You_can_not_make_a_new_test_in_the_course_X_until,
               Gbl.CurrentCrs.Crs.FullName);
      fprintf (Gbl.F.Out,": "
	                 "<span id=\"date_next_test\">"
	                 "</span>"
                         "<script type=\"text/javascript\">"
			 "writeLocalDateHMSFromUTC('date_next_test',%ld,'&nbsp;','%s');"
			 "</script>"
			 "</div>",
	       (long) TimeNextTestUTC,Txt_Today);

      /***** End frame *****/
      Lay_EndRoundFrame ();

      return false;
     }
   return true;
  }

/*****************************************************************************/
/****************************** Update test status ***************************/
/*****************************************************************************/

static void Tst_SetTstStatus (unsigned NumTst,Tst_Status_t TstStatus)
  {
   char Query[512];

   /***** Delete old status from expired sessions *****/
   sprintf (Query,"DELETE FROM tst_status"
                  " WHERE SessionId NOT IN (SELECT SessionId FROM sessions)");
   DB_QueryDELETE (Query,"can not remove old status of tests");

   /***** Update database *****/
   sprintf (Query,"REPLACE INTO tst_status (SessionId,CrsCod,NumTst,Status) VALUES ('%s','%ld','%u','%u')",
            Gbl.Session.Id,Gbl.CurrentCrs.Crs.CrsCod,NumTst,(unsigned) TstStatus);
   DB_QueryREPLACE (Query,"can not update status of test");
  }

/*****************************************************************************/
/****************************** Update test status ***************************/
/*****************************************************************************/

static Tst_Status_t Tst_GetTstStatus (unsigned NumTst)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRows;
   unsigned UnsignedNum;
   Tst_Status_t TstStatus = Tst_STATUS_ERROR;

   /***** Get status of test from database *****/
   sprintf (Query,"SELECT Status FROM tst_status"
                  " WHERE SessionId='%s' AND CrsCod='%ld' AND NumTst='%u'",
            Gbl.Session.Id,Gbl.CurrentCrs.Crs.CrsCod,NumTst);
   NumRows = DB_QuerySELECT (Query,&mysql_res,"can not get status of test");

   if (NumRows == 1)
     {
      /* Get number of hits */
      row = mysql_fetch_row (mysql_res);
      if (row[0])
         if (sscanf (row[0],"%u",&UnsignedNum) == 1)
            if (UnsignedNum < Tst_NUM_STATUS)
               TstStatus = (Tst_Status_t) UnsignedNum;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return TstStatus;
  }

/*****************************************************************************/
/************************* Get number of hits to test ************************/
/*****************************************************************************/

static unsigned Tst_GetNumAccessesTst (void)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRows;
   unsigned NumAccessesTst = 0;

   if (Gbl.Usrs.Me.IBelongToCurrentCrs)
     {
      /***** Get number of hits to test from database *****/
      sprintf (Query,"SELECT NumAccTst FROM crs_usr"
                     " WHERE CrsCod='%ld' AND UsrCod='%ld'",
               Gbl.CurrentCrs.Crs.CrsCod,Gbl.Usrs.Me.UsrDat.UsrCod);
      NumRows = DB_QuerySELECT (Query,&mysql_res,"can not get number of hits to test");

      if (NumRows == 0)
         NumAccessesTst = 0;
      else if (NumRows == 1)
        {
         /* Get number of hits */
         row = mysql_fetch_row (mysql_res);
         if (row[0] == NULL)
            NumAccessesTst = 0;
         else if (sscanf (row[0],"%u",&NumAccessesTst) != 1)
            NumAccessesTst = 0;
        }
      else
         Lay_ShowErrorAndExit ("Error when getting number of hits to test.");

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }

   return NumAccessesTst;
  }

/*****************************************************************************/
/*************************** Write the test questions ************************/
/*****************************************************************************/
// NumRows must hold the number of rows of a MySQL query
// In each row mysql_res holds: in the column 0 the code of a question, in the column 1 the type of answer, and in the column 2 the stem

static void Tst_ShowTestQuestionsWhenSeeing (MYSQL_RES *mysql_res)
  {
   unsigned NumQst;
   long QstCod;
   MYSQL_ROW row;
   double ScoreThisQst;		// Not used here
   bool AnswerIsNotBlank;	// Not used here

   /***** Write rows *****/
   for (NumQst = 0;
	NumQst < Gbl.Test.NumQsts;
	NumQst++)
     {
      Gbl.RowEvenOdd = NumQst % 2;

      /***** Get the row next of the result of the query in the database *****/
      row = mysql_fetch_row (mysql_res);

      /***** Get the code of question (row[0]) *****/
      if ((QstCod = Str_ConvertStrCodToLongCod (row[0])) < 0)
         Lay_ShowErrorAndExit ("Wrong code of question.");

      Tst_WriteQstAndAnsExam (NumQst,QstCod,row,
	                      &ScoreThisQst,		// Not used here
	                      &AnswerIsNotBlank);	// Not used here
     }
  }

/*****************************************************************************/
/************************ Show test tags in this exam ************************/
/*****************************************************************************/

static void Tst_ShowTstTagsPresentInAnExam (long TstCod)
  {
   extern const char *Txt_no_tags;
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRows;
   unsigned long NumRow;

   /***** Get all tags of questions in this test exam *****/
   sprintf (Query,"SELECT tst_tags.TagTxt FROM"
	          " (SELECT DISTINCT(tst_question_tags.TagCod)"
	          " FROM tst_question_tags,tst_exam_questions"
	          " WHERE tst_exam_questions.TstCod='%ld'"
	          " AND tst_exam_questions.QstCod=tst_question_tags.QstCod)"
	          " AS TagsCods,tst_tags"
	          " WHERE TagsCods.TagCod=tst_tags.TagCod"
	          " ORDER BY tst_tags.TagTxt",
            TstCod);
   if ((NumRows = DB_QuerySELECT (Query,&mysql_res,"can not get tags present in a test exam")))
     {
      /***** Write the tags *****/
      fprintf (Gbl.F.Out,"<ul>");
      for (NumRow = 0;
	   NumRow < NumRows;
	   NumRow++)
        {
         row = mysql_fetch_row (mysql_res);
         fprintf (Gbl.F.Out,"<li>%s</li>",
                  row[0]);
        }
      fprintf (Gbl.F.Out,"</ul>");
     }
   else
      fprintf (Gbl.F.Out,"%s",
               Txt_no_tags);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/******************* Show the result of assessing a test *********************/
/*****************************************************************************/

static void Tst_ShowTstResultAfterAssess (long TstCod,unsigned *NumQstsNotBlank,double *TotalScore)
  {
   extern const char *Txt_Question_removed;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumQst;
   long QstCod;
   double ScoreThisQst;
   bool AnswerIsNotBlank;

   /***** Initialize score and number of questions not blank *****/
   *TotalScore = 0.0;
   *NumQstsNotBlank = 0;

   for (NumQst = 0;
	NumQst < Gbl.Test.NumQsts;
	NumQst++)
     {
      Gbl.RowEvenOdd = NumQst % 2;

      /***** Query database *****/
      if (Tst_GetOneQuestionByCod (Gbl.Test.QstCodes[NumQst],&mysql_res))	// Question exists
	{
	 /***** Get row of the result of the query *****/
	 row = mysql_fetch_row (mysql_res);
	 /*
	 row[ 0] QstCod
	 row[ 1] UNIX_TIMESTAMP(EditTime)
	 row[ 2] AnsType
	 row[ 3] Shuffle
	 row[ 4] Stem
	 row[ 5] Feedback
	 row[ 6] ImageName
	 row[ 7] ImageTitle
	 row[ 8] NumHits
	 row[ 9] NumHitsNotBlank
	 row[10] Score
	 */

	 /***** Get the code of question (row[0]) *****/
	 if ((QstCod = Str_ConvertStrCodToLongCod (row[0])) < 0)
	    Lay_ShowErrorAndExit ("Wrong code of question.");

	 /***** Write questions and answers *****/
	 Tst_WriteQstAndAnsExam (NumQst,QstCod,row,
				 &ScoreThisQst,&AnswerIsNotBlank);

	 /***** Store exam questions in database *****/
	 Tst_StoreOneExamQstInDB (TstCod,QstCod,
				  NumQst,	// 0, 1, 2, 3...
				  ScoreThisQst);

	 /***** Compute total score *****/
	 *TotalScore += ScoreThisQst;
	 if (AnswerIsNotBlank)
	    (*NumQstsNotBlank)++;

	 /***** Update the number of accesses and the score of this question *****/
	 if (Gbl.Usrs.Me.LoggedRole == Rol_STUDENT)
	    Tst_UpdateScoreQst (QstCod,ScoreThisQst,AnswerIsNotBlank);
	}
      else
	 /***** Question does not exists *****/
         fprintf (Gbl.F.Out,"<tr>"
	                    "<td class=\"TEST_NUM_QST RIGHT_TOP COLOR%u\">"
	                    "%u"
	                    "</td>"
	                    "<td class=\"DAT_LIGHT LEFT_TOP COLOR%u\">"
	                    "%s"
	                    "</td>"
	                    "</tr>",
                  Gbl.RowEvenOdd,NumQst + 1,
                  Gbl.RowEvenOdd,Txt_Question_removed);

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
  }

/*****************************************************************************/
/******** Write a row of a test exam, with one question and its answer *******/
/*****************************************************************************/

static void Tst_WriteQstAndAnsExam (unsigned NumQst,long QstCod,MYSQL_ROW row,
                                    double *ScoreThisQst,bool *AnswerIsNotBlank)
  {
   extern const char *Txt_TST_STR_ANSWER_TYPES[Tst_NUM_ANS_TYPES];
   /*
   row[ 0] QstCod
   row[ 1] UNIX_TIMESTAMP(EditTime)
   row[ 2] AnsType
   row[ 3] Shuffle
   row[ 4] Stem
   row[ 5] Feedback
   row[ 6] ImageName
   row[ 7] ImageTitle
   row[ 8] NumHits
   row[ 9] NumHitsNotBlank
   row[10] Score
   */

   /***** Create test question *****/
   Tst_QstConstructor ();
   Gbl.Test.QstCod = QstCod;

   /***** Write number of question *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"RIGHT_TOP COLOR%u\">"
	              "<div class=\"TEST_NUM_QST\">%u</div>",
            Gbl.RowEvenOdd,
            NumQst + 1);

   /***** Write answer type (row[2]) *****/
   Gbl.Test.AnswerType = Tst_ConvertFromStrAnsTypDBToAnsTyp (row[2]);
   fprintf (Gbl.F.Out,"<div class=\"DAT_SMALL\">%s</div>"
	              "</td>",
            Txt_TST_STR_ANSWER_TYPES[Gbl.Test.AnswerType]);

   /***** Write stem (row[4]), image (row[6], row[7]),
          answers depending on shuffle (row[3]) and feedback (row[5])  *****/
   fprintf (Gbl.F.Out,"<td class=\"LEFT_TOP COLOR%u\">",
            Gbl.RowEvenOdd);
   Tst_WriteQstStem (row[4],"TEST_EXA");
   Img_GetImageNameAndTitleFromRow (row[6],row[7],&Gbl.Test.Image);
   Img_ShowImage (&Gbl.Test.Image,"TEST_IMG_SHOW_STEM");

   if (Gbl.Action.Act == ActSeeTst)
      Tst_WriteAnswersOfAQstSeeExam (NumQst,QstCod,(Str_ConvertToUpperLetter (row[3][0]) == 'Y'));
   else	// Assessing exam / Viewing old exam
     {
      Tst_WriteAnswersOfAQstAssessExam (NumQst,QstCod,ScoreThisQst,AnswerIsNotBlank);

      /* Write question feedback (row[5]) */
      if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
         Tst_WriteQstFeedback (row[5],"TEST_EXA_LIGHT");
     }
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Destroy test question *****/
   Tst_QstDestructor ();
  }

/*****************************************************************************/
/********************* Write the stem of a test question *********************/
/*****************************************************************************/

void Tst_WriteQstStem (const char *Stem,const char *ClassStem)
  {
   unsigned long StemLength;
   char *StemRigorousHTML;

   /***** Convert the stem, that is in HTML, to rigorous HTML *****/
   StemLength = strlen (Stem) * Str_MAX_LENGTH_SPEC_CHAR_HTML;
   if ((StemRigorousHTML = malloc (StemLength+1)) == NULL)
      Lay_ShowErrorAndExit ("Not enough memory to store stem of question.");
   strncpy (StemRigorousHTML,Stem,StemLength);
   StemRigorousHTML[StemLength] = '\0';
   Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
	             StemRigorousHTML,StemLength,false);

   /***** Write the stem *****/
   fprintf (Gbl.F.Out,"<div class=\"%s\">"
	              "%s"
	              "</div>",
	    ClassStem,StemRigorousHTML);

   /***** Free memory allocated for the stem *****/
   free ((void *) StemRigorousHTML);
  }

/*****************************************************************************/
/************* Put form to upload a new image for a test question ************/
/*****************************************************************************/

static void Tst_PutFormToEditQstImage (struct Image *Image,
                                       const char *ClassImg,const char *ClassTitle,
                                       const char *ParamAction,
                                       const char *ParamFile,
                                       const char *ParamTitle,
                                       bool OptionsDisabled)
  {
   extern const char *The_ClassForm[The_NUM_THEMES];
   extern const char *Txt_No_image;
   extern const char *Txt_Current_image;
   extern const char *Txt_Change_image;
   extern const char *Txt_Image;
   extern const char *Txt_optional;
   extern const char *Txt_Image_title_attribution;
   static unsigned UniqueId = 0;

   /***** Start container *****/
   fprintf (Gbl.F.Out,"<div class=\"LEFT_TOP\">");

   /***** Choice 1: No image *****/
   fprintf (Gbl.F.Out,"<input type=\"radio\" name=\"%s\" value=\"%u\"",
            ParamAction,Img_ACTION_NO_IMAGE);
   if (!Image->Name[0])
      fprintf (Gbl.F.Out," checked=\"checked\"");
   if (OptionsDisabled)
      fprintf (Gbl.F.Out," disabled=\"disabled\"");
   fprintf (Gbl.F.Out," />"
	              "<label class=\"%s\">"
		      "%s"
		      "</label><br />",
	    The_ClassForm[Gbl.Prefs.Theme],
            Txt_No_image);

   /***** Choice 2: Current image *****/
   if (Image->Name[0])
     {
      fprintf (Gbl.F.Out,"<input type=\"radio\" name=\"%s\" value=\"%u\" checked=\"checked\"",
	       ParamAction,Img_ACTION_KEEP_IMAGE);
      if (OptionsDisabled)
         fprintf (Gbl.F.Out," disabled=\"disabled\"");
      fprintf (Gbl.F.Out," />"
	                 "<label class=\"%s\">"
			 "%s"
			 "</label><br />",
	       The_ClassForm[Gbl.Prefs.Theme],
	       Txt_Current_image);
      Img_ShowImage (Image,ClassImg);
     }

   /***** Choice 3: Change/new image *****/
   UniqueId++;
   if (Image->Name[0])	// Image exists
     {
      /***** Change image *****/
      fprintf (Gbl.F.Out,"<input type=\"radio\" id=\"chg_img_%u\" name=\"%s\""
			 " value=\"%u\"",
	       UniqueId,ParamAction,Img_ACTION_CHANGE_IMAGE);	// Replace existing image by new image
      if (OptionsDisabled)
         fprintf (Gbl.F.Out," disabled=\"disabled\"");
      fprintf (Gbl.F.Out," />"
	                 "<label class=\"%s\">"
			 "%s: "
			 "</label>",
	       The_ClassForm[Gbl.Prefs.Theme],Txt_Change_image);
     }
   else			// Image does not exist
     {
      /***** New image *****/
      fprintf (Gbl.F.Out,"<input type=\"radio\" id=\"chg_img_%u\" name=\"%s\""
			 " value=\"%u\"",
	       UniqueId,ParamAction,Img_ACTION_NEW_IMAGE);	// Upload new image
      if (OptionsDisabled)
         fprintf (Gbl.F.Out," disabled=\"disabled\"");
      fprintf (Gbl.F.Out," />"
                         "<label class=\"%s\">"
			 "%s (%s): "
			 "</label>",
	       The_ClassForm[Gbl.Prefs.Theme],Txt_Image,Txt_optional);
     }

   /***** Image file *****/
   fprintf (Gbl.F.Out,"<input type=\"file\" name=\"%s\""
		      " size=\"40\" maxlength=\"100\" value=\"\"",
	    ParamFile);
   if (OptionsDisabled)
      fprintf (Gbl.F.Out," disabled=\"disabled\"");
   fprintf (Gbl.F.Out," onchange=\"document.getElementById('chg_img_%u').checked = true;\" />"
	              "<br />",
	    UniqueId);

   /***** Image title/attribution *****/
   fprintf (Gbl.F.Out,"<label class=\"%s\">"
		      "%s (%s):"
		      "</label><br />"
                      "<input type=\"text\" name=\"%s\" class=\"%s\""
                      " maxlength=\"%u\" value=\"%s\">",
            The_ClassForm[Gbl.Prefs.Theme],
            Txt_Image_title_attribution,Txt_optional,
            ParamTitle,ClassTitle,
            Img_MAX_BYTES_TITLE,Image->Title ? Image->Title : "");

   /***** End container *****/
   fprintf (Gbl.F.Out,"</div>");
  }

/*****************************************************************************/
/******************* Write the feedback of a test question *******************/
/*****************************************************************************/

void Tst_WriteQstFeedback (const char *Feedback,const char *ClassFeedback)
  {
   unsigned long FeedbackLength;
   char *FeedbackRigorousHTML;

   if (Feedback)
      if (Feedback[0])
	{
	 /***** Convert the feedback, that is in HTML, to rigorous HTML *****/
	 FeedbackLength = strlen (Feedback) * Str_MAX_LENGTH_SPEC_CHAR_HTML;
	 if ((FeedbackRigorousHTML = malloc (FeedbackLength+1)) == NULL)
	    Lay_ShowErrorAndExit ("Not enough memory to store stem of question.");
	 strncpy (FeedbackRigorousHTML,Feedback,FeedbackLength);
	 FeedbackRigorousHTML[FeedbackLength] = '\0';
	 Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
			   FeedbackRigorousHTML,FeedbackLength,false);

	 /***** Write the feedback *****/
	 fprintf (Gbl.F.Out,"<div class=\"%s\">"
			    "%s"
			    "</div>",
		  ClassFeedback,FeedbackRigorousHTML);

	 /***** Free memory allocated for the feedback *****/
	 free ((void *) FeedbackRigorousHTML);
	}
  }

/*****************************************************************************/
/*********************** Update the score of a question **********************/
/*****************************************************************************/

static void Tst_UpdateScoreQst (long QstCod,float ScoreThisQst,bool AnswerIsNotBlank)
  {
   char Query[512];

   /***** Update number of clicks and score of the question *****/
   setlocale (LC_NUMERIC,"en_US.utf8");	// To print the floating point as a dot
   if (AnswerIsNotBlank)
      sprintf (Query,"UPDATE tst_questions"
	             " SET NumHits=NumHits+1,NumHitsNotBlank=NumHitsNotBlank+1,Score=Score+(%lf)"
                     " WHERE QstCod='%ld'",
               ScoreThisQst,QstCod);
   else	// The answer is blank
      sprintf (Query,"UPDATE tst_questions"
	             " SET NumHits=NumHits+1"
                     " WHERE QstCod='%ld'",
               QstCod);
   setlocale (LC_NUMERIC,"es_ES.utf8");	// Return to spanish system (TODO: this should be internationalized!!!!!!!)
   DB_QueryUPDATE (Query,"can not update the score of a question");
  }

/*****************************************************************************/
/*********** Update my number of accesses to test in this course *************/
/*****************************************************************************/

static void Tst_UpdateMyNumAccessTst (unsigned NumAccessesTst)
  {
   char Query[512];

   /***** Update my number of accesses to test in this course *****/
   sprintf (Query,"UPDATE crs_usr SET NumAccTst='%u'"
                  " WHERE CrsCod='%ld' AND UsrCod='%ld'",
	    NumAccessesTst,
	    Gbl.CurrentCrs.Crs.CrsCod,Gbl.Usrs.Me.UsrDat.UsrCod);
   DB_QueryUPDATE (Query,"can not update the number of accesses to test");
  }

/*****************************************************************************/
/************ Update date-time of my next allowed access to test *************/
/*****************************************************************************/

static void Tst_UpdateLastAccTst (void)
  {
   char Query[512];

   /***** Update date-time and number of questions of this test *****/
   sprintf (Query,"UPDATE crs_usr SET LastAccTst=NOW(),NumQstsLastTst='%u'"
                  " WHERE CrsCod='%ld' AND UsrCod='%ld'",
            Gbl.Test.NumQsts,
	    Gbl.CurrentCrs.Crs.CrsCod,
	    Gbl.Usrs.Me.UsrDat.UsrCod);
   DB_QueryUPDATE (Query,"can not update time and number of questions of this test");
  }

/*****************************************************************************/
/************ Set end date to current date                        ************/
/************ and set initial date to end date minus several days ************/
/*****************************************************************************/

void Tst_SetIniEndDates (void)
  {
   Gbl.DateRange.TimeUTC[0] = (time_t) 0;
   Gbl.DateRange.TimeUTC[1] = Gbl.StartExecutionTimeUTC;
  }

/*****************************************************************************/
/******* Select tags and dates for edition of the self-assessment test *******/
/*****************************************************************************/

void Tst_ShowFormAskEditTsts (void)
  {
   extern const char *Txt_No_test_questions;
   extern const char *Txt_List_edit_questions;
   extern const char *Txt_Show_questions;
   MYSQL_RES *mysql_res;
   unsigned long NumRows;

   /***** Contextual menu *****/
   fprintf (Gbl.F.Out,"<div class=\"CONTEXT_MENU\">");
   TsI_PutFormToImportQuestions ();	// Put link (form) to import questions from XML file
   Tst_PutFormToConfigure ();		// Put form to go to test configuration
   fprintf (Gbl.F.Out,"</div>");

   /***** Start frame *****/
   Lay_StartRoundFrame (NULL,Txt_List_edit_questions,Tst_PutIconToCreateNewTstQst);

   /***** Get tags already present in the table of questions *****/
   if ((NumRows = Tst_GetAllTagsFromCurrentCrs (&mysql_res)))
     {
      Act_FormStart (ActLstTstQst);
      Par_PutHiddenParamUnsigned ("Order",(unsigned) Tst_ORDER_STEM);

      fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">");

      /***** Selection of tags *****/
      Tst_ShowFormSelTags (NumRows,mysql_res,false,2);

      /***** Selection of types of answers *****/
      Tst_ShowFormAnswerTypes (2);

      /***** Starting and ending dates in the search *****/
      Dat_PutFormStartEndClientLocalDateTimesWithYesterdayToday ();
      fprintf (Gbl.F.Out,"</table>");

      /***** Send button *****/
      Lay_PutConfirmButton (Txt_Show_questions);
      Act_FormEnd ();
     }
   else	// No test questions
     {
      /***** Warning message *****/
      Lay_ShowAlert (Lay_INFO,Txt_No_test_questions);

      /***** Button to create a new question *****/
      Tst_PutButtonToAddQuestion ();
     }

   /***** End frame *****/
   Lay_EndRoundFrame ();

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/***************** Put icon to create a new test question ********************/
/*****************************************************************************/

static void Tst_PutIconToCreateNewTstQst (void)
  {
   extern const char *Txt_New_question;

   /***** Put form to create a new test question *****/
   Lay_PutContextualLink (ActEdiOneTstQst,NULL,"plus64x64.png",
                          Txt_New_question,NULL);
  }

/*****************************************************************************/
/**************** Put button to create a new test question *******************/
/*****************************************************************************/

static void Tst_PutButtonToAddQuestion (void)
  {
   extern const char *Txt_New_question;

   Act_FormStart (ActEdiOneTstQst);
   Lay_PutConfirmButton (Txt_New_question);
   Act_FormEnd ();
  }

/*****************************************************************************/
/***************************** Form to rename tags ***************************/
/*****************************************************************************/

void Tst_ShowFormConfig (void)
  {
   extern const char *Txt_Please_specify_if_you_allow_access_to_test_questions_from_mobile_applications;

   /***** If current course has tests and pluggable is unknown... *****/
   if (Tst_CheckIfCourseHaveTestsAndPluggableIsUnknown ())
      Lay_ShowAlert (Lay_WARNING,Txt_Please_specify_if_you_allow_access_to_test_questions_from_mobile_applications);

   /***** Form to configure test *****/
   Tst_ShowFormConfigTst ();

   /***** Form to edit tags *****/
   Tst_ShowFormEditTags ();
  }

/*****************************************************************************/
/******************************* Enable a test tag ***************************/
/*****************************************************************************/

void Tst_EnableTag (void)
  {
   long TagCod = Tst_GetParamTagCode ();

   /***** Change tag status to enabled *****/
   Tst_EnableOrDisableTag (TagCod,false);

   /***** Show again the form to configure test *****/
   Tst_ShowFormConfig ();
  }

/*****************************************************************************/
/****************************** Disable a test tag ***************************/
/*****************************************************************************/

void Tst_DisableTag (void)
  {
   long TagCod = Tst_GetParamTagCode ();

   /***** Change tag status to disabled *****/
   Tst_EnableOrDisableTag (TagCod,true);

   /***** Show again the form to configure test *****/
   Tst_ShowFormConfig ();
  }

/*****************************************************************************/
/************************* Get parameter with tag code ***********************/
/*****************************************************************************/

static long Tst_GetParamTagCode (void)
  {
   char StrLong[1+10+1];
   long TagCod;

   Par_GetParToText ("TagCod",StrLong,1+10);
   if ((TagCod = Str_ConvertStrCodToLongCod (StrLong)) < 0)
      Lay_ShowErrorAndExit ("Code of tag is missing.");

   return TagCod;
  }

/*****************************************************************************/
/************************ Rename a tag of test questions *********************/
/*****************************************************************************/

void Tst_RenameTag (void)
  {
   extern const char *Txt_You_can_not_leave_the_name_of_the_tag_X_empty;
   extern const char *Txt_The_tag_X_has_been_renamed_as_Y;
   extern const char *Txt_The_tag_X_has_not_changed;
   char OldTagTxt[Tst_MAX_BYTES_TAG+1];
   char NewTagTxt[Tst_MAX_BYTES_TAG+1];
   char Query[1024+2*Tst_MAX_BYTES_TAG];
   long ExistingNewTagCod;
   long OldTagCod;

   /***** Get old and new tags from the form *****/
   Par_GetParToText ("OldTagTxt",OldTagTxt,Tst_MAX_BYTES_TAG);

   Par_GetParToText ("NewTagTxt",NewTagTxt,Tst_MAX_BYTES_TAG);

   /***** Check that the new tag is not empty *****/
   if (!NewTagTxt[0])
     {
      sprintf (Gbl.Message,Txt_You_can_not_leave_the_name_of_the_tag_X_empty,
               OldTagTxt);
      Lay_ShowAlert (Lay_WARNING,Gbl.Message);
     }
   else
     {
      /***** Check if the old tag is equal to the new one (this happens when user press INTRO without changing anything in the form) *****/
      if (strcmp (OldTagTxt,NewTagTxt))	// The tag text has changed
        {
         /***** Check if the new tag text is equal to any of the tag texts present in the database *****/
         if ((ExistingNewTagCod = Tst_GetTagCodFromTagTxt (NewTagTxt)) > 0)	// The new tag was already in database
           {
            /***** Complex update made to not repeat tags *****/
            /* Step 1. Get tag code of the old tag */
            if ((OldTagCod =  Tst_GetTagCodFromTagTxt (OldTagTxt)) < 0)
               Lay_ShowErrorAndExit ("Tag does not exists.");

            /* Step 2: If the new tag existed for a question ==> delete old tag from tst_question_tags; the new tag will remain
                       If the new tag did not exist for a question ==> change old tag to new tag in tst_question_tags */
            sprintf (Query,"DROP TEMPORARY TABLE IF EXISTS tst_question_tags_tmp");
            if (mysql_query (&Gbl.mysql,Query))
               DB_ExitOnMySQLError ("can not remove temporary table");

            sprintf (Query,"CREATE TEMPORARY TABLE tst_question_tags_tmp ENGINE=MEMORY"
        	           " SELECT QstCod FROM tst_question_tags WHERE TagCod='%ld'",
                     ExistingNewTagCod);
            if (mysql_query (&Gbl.mysql,Query))
               DB_ExitOnMySQLError ("can not create temporary table");

            sprintf (Query,"DELETE FROM tst_question_tags"
                           " WHERE TagCod='%ld'"
                           " AND QstCod IN"
                           " (SELECT QstCod FROM tst_question_tags_tmp)",	// New tag existed for a question ==> delete old tag
                     OldTagCod);
            DB_QueryDELETE (Query,"can not remove a tag from some questions");

            sprintf (Query,"UPDATE tst_question_tags"
                           " SET TagCod='%ld'"
                           " WHERE TagCod='%ld'"
                           " AND QstCod NOT IN"
                           " (SELECT QstCod FROM tst_question_tags_tmp)",	// New tag did not exist for a question ==> change old tag to new tag
                     ExistingNewTagCod,
                     OldTagCod);
            DB_QueryUPDATE (Query,"can not update a tag in some questions");

            sprintf (Query,"DROP TEMPORARY TABLE IF EXISTS tst_question_tags_tmp");
            if (mysql_query (&Gbl.mysql,Query))
               DB_ExitOnMySQLError ("can not remove temporary table");

            /* Delete old tag from tst_tags because it is not longer used */
            sprintf (Query,"DELETE FROM tst_tags WHERE TagCod='%ld'",
                     OldTagCod);
            DB_QueryDELETE (Query,"can not remove old tag");
            // Tst_RemoveUnusedTagsFromCurrentCrs ();
           }
         else	// Tag is not repeated. New tag text does not exists in current tags
           {
            /***** Simple update replacing each instance of the old tag by the new tag *****/
            sprintf (Query,"UPDATE tst_tags SET TagTxt='%s',ChangeTime=NOW()"
                           " WHERE tst_tags.CrsCod='%ld' AND tst_tags.TagTxt='%s'",
                     NewTagTxt,Gbl.CurrentCrs.Crs.CrsCod,OldTagTxt);
            DB_QueryUPDATE (Query,"can not update tag");
           }

         /***** Write message to show the change made *****/
         sprintf (Gbl.Message,Txt_The_tag_X_has_been_renamed_as_Y,
                  OldTagTxt,NewTagTxt);
         Lay_ShowAlert (Lay_SUCCESS,Gbl.Message);
        }
      else	// Both tags are the same
        {
         sprintf (Gbl.Message,Txt_The_tag_X_has_not_changed,
                  NewTagTxt);
         Lay_ShowAlert (Lay_INFO,Gbl.Message);
        }
     }

   /***** Show again the form to configure test *****/
   Tst_ShowFormConfig ();
  }

/*****************************************************************************/
/******************* Check if current course has test tags *******************/
/*****************************************************************************/
// Return the number of rows of the result

static bool Tst_CheckIfCurrentCrsHasTestTags (void)
  {
   char Query[512];

   /***** Get available tags from database *****/
   sprintf (Query,"SELECT COUNT(*) FROM tst_tags WHERE CrsCod='%ld'",
            Gbl.CurrentCrs.Crs.CrsCod);
   return (DB_QueryCOUNT (Query,"can not check if course has tags") != 0);
  }

/*****************************************************************************/
/********* Get all (enabled or disabled) test tags for this course ***********/
/*****************************************************************************/
// Return the number of rows of the result

static unsigned long Tst_GetAllTagsFromCurrentCrs (MYSQL_RES **mysql_res)
  {
   char Query[512];

   /***** Get available tags from database *****/
   sprintf (Query,"SELECT TagCod,TagTxt,TagHidden FROM tst_tags"
                  " WHERE CrsCod='%ld' ORDER BY TagTxt",
            Gbl.CurrentCrs.Crs.CrsCod);
   return DB_QuerySELECT (Query,mysql_res,"can not get available tags");
  }

/*****************************************************************************/
/********************** Get enabled test tags for this course ****************/
/*****************************************************************************/
// Return the number of rows of the result

static unsigned long Tst_GetEnabledTagsFromThisCrs (MYSQL_RES **mysql_res)
  {
   char Query[512];

   /***** Get available not hidden tags from database *****/
   sprintf (Query,"SELECT TagCod,TagTxt FROM tst_tags"
                  " WHERE CrsCod='%ld' AND TagHidden='N' ORDER BY TagTxt",
            Gbl.CurrentCrs.Crs.CrsCod);
   return DB_QuerySELECT (Query,mysql_res,"can not get available enabled tags");
  }

/*****************************************************************************/
/********************* Show a form to select test tags ***********************/
/*****************************************************************************/

static void Tst_ShowFormSelTags (unsigned long NumRows,MYSQL_RES *mysql_res,
                                 bool ShowOnlyEnabledTags,unsigned NumCols)
  {
   extern const char *The_ClassForm[The_NUM_THEMES];
   extern const char *Txt_Tags;
   extern const char *Txt_All_tags;
   extern const char *Txt_Tag_not_allowed;
   extern const char *Txt_Tag_allowed;
   unsigned long NumRow;
   MYSQL_ROW row;
   bool TagHidden = false;
   const char *Ptr;
   char TagText[Tst_MAX_BYTES_TAG+1];

   /***** Label *****/
   fprintf (Gbl.F.Out,"<tr>"
		      "<td class=\"RIGHT_TOP\">"
		      "<label class=\"%s\">"
		      "%s:"
		      "</label>"
                      "</td>",
	    The_ClassForm[Gbl.Prefs.Theme],Txt_Tags);

   /***** Select all tags *****/
   fprintf (Gbl.F.Out,"<td");
   if (NumCols > 1)
      fprintf (Gbl.F.Out," colspan=\"%u\"",NumCols);
   fprintf (Gbl.F.Out," class=\"LEFT_TOP\">"
	              "<table class=\"CELLS_PAD_2\">"
                      "<tr>");
   if (!ShowOnlyEnabledTags)
      fprintf (Gbl.F.Out,"<td></td>");
   fprintf (Gbl.F.Out,"<td class=\"%s LEFT_MIDDLE\">"
	              "<input type=\"checkbox\" name=\"AllTags\" value=\"Y\"",
            The_ClassForm[Gbl.Prefs.Theme]);
   if (Gbl.Test.Tags.All)
    fprintf (Gbl.F.Out," checked=\"checked\"");
   fprintf (Gbl.F.Out," onclick=\"togglecheckChildren(this,'ChkTag')\" />"
                      " %s"
                      "</td>"
                      "</tr>",
            Txt_All_tags);

   /***** Select tags one by one *****/
   for (NumRow = 1;
	NumRow <= NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);
      fprintf (Gbl.F.Out,"<tr>");
      if (!ShowOnlyEnabledTags)
        {
         TagHidden = (Str_ConvertToUpperLetter (row[2][0]) == 'Y');
         fprintf (Gbl.F.Out,"<td class=\"LEFT_MIDDLE\">"
                            "<img src=\"%s/",
                  Gbl.Prefs.IconsURL);
         if (TagHidden)
            fprintf (Gbl.F.Out,"eye-slash-off64x64.png\" alt=\"%s\" title=\"%s",
                     Txt_Tag_not_allowed,
                     Txt_Tag_not_allowed);
         else
            fprintf (Gbl.F.Out,"eye-off64x64.png\" alt=\"%s\" title=\"%s",
                     Txt_Tag_allowed,
                     Txt_Tag_allowed);
         fprintf (Gbl.F.Out,"\" class=\"ICON20x20\" />"
                            "</td>");
        }
      fprintf (Gbl.F.Out,"<td class=\"DAT LEFT_MIDDLE\">"
	                 "<input type=\"checkbox\" name=\"ChkTag\" value=\"%s\"",
	       row[1]);
      if (Gbl.Test.Tags.List)
        {
         Ptr = Gbl.Test.Tags.List;
         while (*Ptr)
           {
            Par_GetNextStrUntilSeparParamMult (&Ptr,TagText,Tst_MAX_BYTES_TAG);
            if (!strcmp (row[1],TagText))
               fprintf (Gbl.F.Out," checked=\"checked\"");
           }
        }
      fprintf (Gbl.F.Out," onclick=\"checkParent(this,'AllTags')\" />"
	                 " %s"
	                 "</td>"
	                 "</tr>",
	       row[1]);
     }

   fprintf (Gbl.F.Out,"</table>"
	              "</td>"
	              "</tr>");
  }

/*****************************************************************************/
/************* Show a form to enable/disable and rename test tags ************/
/*****************************************************************************/

static void Tst_ShowFormEditTags (void)
  {
   extern const char *Txt_No_test_questions;
   extern const char *Txt_Tags;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRow,NumRows;
   long TagCod;

   /***** Get current tags in current course *****/
   if ((NumRows = Tst_GetAllTagsFromCurrentCrs (&mysql_res)))
     {
      /***** Start table *****/
      Lay_StartRoundFrameTable (NULL,2,Txt_Tags);

      /***** Show tags *****/
      for (NumRow = 0;
	   NumRow < NumRows;
	   NumRow++)
        {
         row = mysql_fetch_row (mysql_res);
         if ((TagCod = Str_ConvertStrCodToLongCod (row[0])) < 0)
            Lay_ShowErrorAndExit ("Wrong code of tag.");

         fprintf (Gbl.F.Out,"<tr>");

         /* Form to enable / disable this tag */
         if (Str_ConvertToUpperLetter (row[2][0]) == 'Y')	// Tag disabled
            Tst_PutIconEnable (TagCod,row[1]);
         else
            Tst_PutIconDisable (TagCod,row[1]);

         /* Form to rename this tag */
         fprintf (Gbl.F.Out,"<td class=\"LEFT_MIDDLE\">");
         Act_FormStart (ActRenTag);
         Par_PutHiddenParamString ("OldTagTxt",row[1]);
         fprintf (Gbl.F.Out,"<input type=\"text\" name=\"NewTagTxt\""
                            " size=\"36\" maxlength=\"%u\" value=\"%s\""
                            " onchange=\"document.getElementById('%s').submit();\" />",
                  Tst_MAX_TAG_LENGTH,row[1],Gbl.Form.Id);
         Act_FormEnd ();
         fprintf (Gbl.F.Out,"</td>"
                            "</tr>");
        }

      /***** End table *****/
      Lay_EndRoundFrameTable ();
     }
   else
      Lay_ShowAlert (Lay_INFO,Txt_No_test_questions);

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/******************* Put a link and an icon to enable a tag ******************/
/*****************************************************************************/

static void Tst_PutIconEnable (long TagCod,const char *TagTxt)
  {
   extern const char *Txt_Tag_X_not_allowed_Click_to_allow_it;

   fprintf (Gbl.F.Out,"<td class=\"BM\">");
   Act_FormStart (ActEnableTag);
   Par_PutHiddenParamLong ("TagCod",TagCod);
   sprintf (Gbl.Title,Txt_Tag_X_not_allowed_Click_to_allow_it,TagTxt);
   fprintf (Gbl.F.Out,"<input type=\"image\" src=\"%s/eye-slash-on64x64.png\""
	              " alt=\"%s\" title=\"%s\""
	              " class=\"ICON20x20\" />",
            Gbl.Prefs.IconsURL,
            Gbl.Title,
            Gbl.Title);
   Act_FormEnd ();
   fprintf (Gbl.F.Out,"</td>");
  }

/*****************************************************************************/
/****************** Put a link and an icon to disable a tag ******************/
/*****************************************************************************/

static void Tst_PutIconDisable (long TagCod,const char *TagTxt)
  {
   extern const char *Txt_Tag_X_allowed_Click_to_disable_it;

   fprintf (Gbl.F.Out,"<td class=\"BM\">");
   Act_FormStart (ActDisableTag);
   Par_PutHiddenParamLong ("TagCod",TagCod);
   sprintf (Gbl.Title,Txt_Tag_X_allowed_Click_to_disable_it,TagTxt);
   fprintf (Gbl.F.Out,"<input type=\"image\" src=\"%s/eye-on64x64.png\""
	              " alt=\"%s\" title=\"%s\""
	              " class=\"ICON20x20\" />",
            Gbl.Prefs.IconsURL,
            Gbl.Title,
            Gbl.Title);
   Act_FormEnd ();
   fprintf (Gbl.F.Out,"</td>");
  }

/*****************************************************************************/
/********************* Show a form to to configure test **********************/
/*****************************************************************************/

static void Tst_ShowFormConfigTst (void)
  {
   extern const char *The_ClassForm[The_NUM_THEMES];
   extern const char *Txt_Configure_tests;
   extern const char *Txt_Plugins;
   extern const char *Txt_TST_PLUGGABLE[Tst_NUM_OPTIONS_PLUGGABLE];
   extern const char *Txt_No_of_questions;
   extern const char *Txt_minimum;
   extern const char *Txt_default;
   extern const char *Txt_maximum;
   extern const char *Txt_Minimum_time_seconds_per_question_between_two_tests;
   extern const char *Txt_Feedback_to_students;
   extern const char *Txt_TST_STR_FEEDBACK[Tst_NUM_TYPES_FEEDBACK];
   extern const char *Txt_Save;
   Tst_Pluggable_t Pluggable;
   Tst_Feedback_t FeedbTyp;

   /***** Read test configuration from database *****/
   Tst_GetConfigTstFromDB ();

   /***** Start form *****/
   Act_FormStart (ActRcvCfgTst);
   Lay_StartRoundFrameTable (NULL,2,Txt_Configure_tests);

   /***** Tests are visible from plugins? *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"%s RIGHT_TOP\">"
	              "%s:"
	              "</td>"
	              "<td class=\"LEFT_TOP\">",
            The_ClassForm[Gbl.Prefs.Theme],
            Txt_Plugins);
   for (Pluggable = Tst_PLUGGABLE_NO;
	Pluggable <= Tst_PLUGGABLE_YES;
	Pluggable++)
     {
      fprintf (Gbl.F.Out,"<input type=\"radio\" name=\"Pluggable\" value=\"%u\"",
	       (unsigned) Pluggable);
      if (Pluggable == Gbl.Test.Config.Pluggable)
         fprintf (Gbl.F.Out," checked=\"checked\"");
      fprintf (Gbl.F.Out," /><span class=\"DAT\">%s</span><br />",
               Txt_TST_PLUGGABLE[Pluggable]);
     }
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Number of questions *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"%s RIGHT_TOP\">"
	              "%s:"
	              "</td>"
                      "<td class=\"LEFT_TOP\">"
                      "<table style=\"border-spacing:2px;\">",
            The_ClassForm[Gbl.Prefs.Theme],
            Txt_No_of_questions);

   /* Minimum number of questions in a test exam */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"DAT RIGHT_MIDDLE\">"
	              "%s"
	              "</td>"
                      "<td class=\"LEFT_MIDDLE\">"
                      "<input type=\"text\" name=\"NumQstMin\""
                      " size=\"3\" maxlength=\"3\" value=\"%u\" />"
                      "</td>"
                      "</tr>",
            Txt_minimum,Gbl.Test.Config.Min);

   /* Default number of questions in a test exam */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"DAT RIGHT_MIDDLE\">"
	              "%s"
	              "</td>"
                      "<td class=\"LEFT_MIDDLE\">"
                      "<input type=\"text\" name=\"NumQstDef\""
                      " size=\"3\" maxlength=\"3\" value=\"%u\" />"
                      "</td>"
                      "</tr>",
            Txt_default,Gbl.Test.Config.Def);

   /* Maximum number of questions in a test exam */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"DAT RIGHT_MIDDLE\">"
	              "%s"
	              "</td>"
                      "<td class=\"LEFT_MIDDLE\">"
                      "<input type=\"text\" name=\"NumQstMax\""
                      " size=\"3\" maxlength=\"3\" value=\"%u\" />"
                      "</td>"
                      "</tr>"
                      "</table>"
                      "</td>"
                      "</tr>",
            Txt_maximum,Gbl.Test.Config.Max);

   /***** Minimum time between test exams per question *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"%s RIGHT_MIDDLE\">"
	              "%s:"
	              "</td>"
                      "<td class=\"LEFT_MIDDLE\">"
                      "<input type=\"text\" name=\"MinTimeNxtTstPerQst\""
                      " size=\"7\" maxlength=\"7\" value=\"%lu\" />"
                      "</td>"
                      "</tr>",
            The_ClassForm[Gbl.Prefs.Theme],
            Txt_Minimum_time_seconds_per_question_between_two_tests,
            Gbl.Test.Config.MinTimeNxtTstPerQst);

   /***** Feedback to students *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"%s RIGHT_TOP\">"
	              "%s:"
	              "</td>"
	              "<td class=\"LEFT_TOP\">",
            The_ClassForm[Gbl.Prefs.Theme],Txt_Feedback_to_students);
   for (FeedbTyp = (Tst_Feedback_t) 0;
	FeedbTyp < Tst_NUM_TYPES_FEEDBACK;
	FeedbTyp++)
     {
      fprintf (Gbl.F.Out,"<input type=\"radio\" name=\"Feedback\" value=\"%u\"",
	       (unsigned) FeedbTyp);
      if (FeedbTyp == Gbl.Test.Config.FeedbackType)
         fprintf (Gbl.F.Out," checked=\"checked\"");
      fprintf (Gbl.F.Out," /><span class=\"DAT\">%s</span><br />",
               Txt_TST_STR_FEEDBACK[FeedbTyp]);
     }
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Send button and end frame *****/
   Lay_EndRoundFrameTableWithButton (Lay_CONFIRM_BUTTON,Txt_Save);

   /***** End form *****/
   Act_FormEnd ();
  }

/*****************************************************************************/
/*************** Get configuration of test for current course ****************/
/*****************************************************************************/

static void Tst_GetConfigTstFromDB (void)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRows;

   /***** Get configuration of test for current course from database *****/
   sprintf (Query,"SELECT Pluggable,Min,Def,Max,MinTimeNxtTstPerQst,Feedback"
                  " FROM tst_config WHERE CrsCod='%ld'",
            Gbl.CurrentCrs.Crs.CrsCod);
   NumRows = DB_QuerySELECT (Query,&mysql_res,"can not get configuration of test");

   Gbl.Test.Config.FeedbackType = Tst_FEEDBACK_DEFAULT;
   Gbl.Test.Config.MinTimeNxtTstPerQst = 0UL;
   if (NumRows == 0)
     {
      Gbl.Test.Config.Pluggable = Tst_PLUGGABLE_UNKNOWN;
      Gbl.Test.Config.Min = Tst_CONFIG_DEFAULT_MIN_QUESTIONS;
      Gbl.Test.Config.Def = Tst_CONFIG_DEFAULT_DEF_QUESTIONS;
      Gbl.Test.Config.Max = Tst_CONFIG_DEFAULT_MAX_QUESTIONS;
     }
   else // NumRows == 1
     {
      /***** Get minimun, default and maximum *****/
      row = mysql_fetch_row (mysql_res);
      Tst_GetConfigFromRow (row);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************ Get configuration values from a database table row *************/
/*****************************************************************************/

void Tst_GetConfigFromRow (MYSQL_ROW row)
  {
   int IntNum;
   long LongNum;
   Tst_Pluggable_t Pluggable;
   Tst_Feedback_t FeedbTyp;

   /***** Get whether test are visible via plugins or not *****/
   Gbl.Test.Config.Pluggable = Tst_PLUGGABLE_UNKNOWN;
   for (Pluggable = Tst_PLUGGABLE_NO;
	Pluggable <= Tst_PLUGGABLE_YES;
	Pluggable++)
      if (!strcmp (row[0],Tst_PluggableDB[Pluggable]))
        {
         Gbl.Test.Config.Pluggable = Pluggable;
         break;
        }

   /***** Get number of questions *****/
   if (sscanf (row[1],"%d",&IntNum) == 1)
      Gbl.Test.Config.Min = (IntNum < 1) ? 1 :
	                                   (unsigned) IntNum;
   else
      Gbl.Test.Config.Min = Tst_CONFIG_DEFAULT_MIN_QUESTIONS;

   if (sscanf (row[2],"%d",&IntNum) == 1)
      Gbl.Test.Config.Def = (IntNum < 1) ? 1 :
	                                   (unsigned) IntNum;
   else
      Gbl.Test.Config.Def = Tst_CONFIG_DEFAULT_DEF_QUESTIONS;

   if (sscanf (row[3],"%d",&IntNum) == 1)
      Gbl.Test.Config.Max = (IntNum < 1) ? 1 :
	                                   (unsigned) IntNum;
   else
      Gbl.Test.Config.Max = Tst_CONFIG_DEFAULT_MAX_QUESTIONS;

   /***** Check and correct numbers *****/
   Tst_CheckAndCorrectNumbersQst ();

   /***** Get minimum time between test exams per question (row[4]) *****/
   if (sscanf (row[4],"%ld",&LongNum) == 1)
      Gbl.Test.Config.MinTimeNxtTstPerQst = (LongNum < 1L) ? 0UL :
	                                                     (unsigned long) LongNum;

   /***** Get feedback type (row[5]) *****/
   for (FeedbTyp = (Tst_Feedback_t) 0;
	FeedbTyp < Tst_NUM_TYPES_FEEDBACK;
	FeedbTyp++)
      if (!strcmp (row[5],Tst_FeedbackDB[FeedbTyp]))
        {
         Gbl.Test.Config.FeedbackType = FeedbTyp;
         break;
        }
  }

/*****************************************************************************/
/*************** Get configuration of test for current course ****************/
/*****************************************************************************/
// Returns true if course has test tags and pluggable is unknown
// Return false if course has no test tags or pluggable is known

bool Tst_CheckIfCourseHaveTestsAndPluggableIsUnknown (void)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRows;
   Tst_Pluggable_t Pluggable;

   /***** Get pluggability of tests for current course from database *****/
   sprintf (Query,"SELECT Pluggable FROM tst_config WHERE CrsCod='%ld'",
            Gbl.CurrentCrs.Crs.CrsCod);
   NumRows = DB_QuerySELECT (Query,&mysql_res,"can not get configuration of test");

   if (NumRows == 0)
      Gbl.Test.Config.Pluggable = Tst_PLUGGABLE_UNKNOWN;
   else // NumRows == 1
     {
      /***** Get whether test are visible via plugins or not *****/
      row = mysql_fetch_row (mysql_res);

      Gbl.Test.Config.Pluggable = Tst_PLUGGABLE_UNKNOWN;
      for (Pluggable = Tst_PLUGGABLE_NO;
	   Pluggable <= Tst_PLUGGABLE_YES;
	   Pluggable++)
         if (!strcmp (row[0],Tst_PluggableDB[Pluggable]))
           {
            Gbl.Test.Config.Pluggable = Pluggable;
            break;
           }
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Get if current course has tests from database *****/
   if (Gbl.Test.Config.Pluggable == Tst_PLUGGABLE_UNKNOWN)
      return Tst_CheckIfCurrentCrsHasTestTags ();	// Return true if course has tests

   return false;	// Pluggable is not unknown
  }

/*****************************************************************************/
/************* Receive configuration of test for current course **************/
/*****************************************************************************/

void Tst_ReceiveConfigTst (void)
  {
   extern const char *Txt_The_test_configuration_has_been_updated;
   char IntStr[1+10+1];
   int IntNum;
   long LongNum;
   char Query[512];

   /***** Get whether test are visible via plugins or not *****/
   Gbl.Test.Config.Pluggable = Tst_GetPluggableFromForm ();

   /***** Get number of questions *****/
   /* Get minimum number of questions */
   Par_GetParToText ("NumQstMin",IntStr,1+10);
   if (sscanf (IntStr,"%d",&IntNum) != 1)
      Lay_ShowErrorAndExit ("Minimum number of questions is missing.");
   Gbl.Test.Config.Min = (IntNum < 1) ? 1 :
	                                (unsigned) IntNum;

   /* Get default number of questions */
   Par_GetParToText ("NumQstDef",IntStr,1+10);
   if (sscanf (IntStr,"%d",&IntNum) != 1)
      Lay_ShowErrorAndExit ("Default number of questions is missing.");
   Gbl.Test.Config.Def = (IntNum < 1) ? 1 :
	                                (unsigned) IntNum;

   /* Get maximum number of questions */
   Par_GetParToText ("NumQstMax",IntStr,1+10);
   if (sscanf (IntStr,"%d",&IntNum) != 1)
      Lay_ShowErrorAndExit ("Maximum number of questions is missing.");
   Gbl.Test.Config.Max = (IntNum < 1) ? 1 :
	                                (unsigned) IntNum;

   /* Check and correct numbers */
   Tst_CheckAndCorrectNumbersQst ();

   /***** Get minimum time between test exams per question *****/
   Par_GetParToText ("MinTimeNxtTstPerQst",IntStr,1+10);
   if (sscanf (IntStr,"%ld",&LongNum) != 1)
      Lay_ShowErrorAndExit ("Minimum time per question between two test is missing.");
   Gbl.Test.Config.MinTimeNxtTstPerQst = (LongNum < 1L) ? 0UL :
	                                                  (unsigned long) LongNum;

   /***** Get type of feedback from form *****/
   Gbl.Test.Config.FeedbackType = Tst_GetFeedbackTypeFromForm ();

   /***** Update database *****/
   sprintf (Query,"REPLACE INTO tst_config (CrsCod,Pluggable,Min,Def,Max,MinTimeNxtTstPerQst,Feedback)"
                  " VALUES ('%ld','%s','%u','%u','%u','%lu','%s')",
            Gbl.CurrentCrs.Crs.CrsCod,
            Tst_PluggableDB[Gbl.Test.Config.Pluggable],
            Gbl.Test.Config.Min,Gbl.Test.Config.Def,Gbl.Test.Config.Max,
            Gbl.Test.Config.MinTimeNxtTstPerQst,
            Tst_FeedbackDB[Gbl.Test.Config.FeedbackType]);
   DB_QueryREPLACE (Query,"can not save configuration of test");

   /***** Show confirmation message *****/
   Lay_ShowAlert (Lay_SUCCESS,Txt_The_test_configuration_has_been_updated);

   /***** Show again the form to configure test *****/
   Tst_ShowFormConfig ();
  }

/*****************************************************************************/
/******************* Get if tests are pluggable from form ********************/
/*****************************************************************************/

static Tst_Pluggable_t Tst_GetPluggableFromForm (void)
  {
   char UnsignedStr[10+1];
   unsigned UnsignedNum;

   Par_GetParToText ("Pluggable",UnsignedStr,10);
   if (UnsignedStr[0])
     {
      if (sscanf (UnsignedStr,"%u",&UnsignedNum) != 1)
         return Tst_PLUGGABLE_UNKNOWN;
      if (UnsignedNum >= Tst_NUM_TYPES_FEEDBACK)
         return Tst_PLUGGABLE_UNKNOWN;
      return (Tst_Pluggable_t) UnsignedNum;
     }
   return Tst_PLUGGABLE_UNKNOWN;
  }

/*****************************************************************************/
/*********************** Get type of feedback from form **********************/
/*****************************************************************************/

static Tst_Feedback_t Tst_GetFeedbackTypeFromForm (void)
  {
   char UnsignedStr[10+1];
   unsigned UnsignedNum;

   Par_GetParToText ("Feedback",UnsignedStr,10);
   if (UnsignedStr[0])
     {
      if (sscanf (UnsignedStr,"%u",&UnsignedNum) != 1)
         return (Tst_Feedback_t) 0;
      if (UnsignedNum >= Tst_NUM_TYPES_FEEDBACK)
         return (Tst_Feedback_t) 0;
      return (Tst_Feedback_t) UnsignedNum;
     }
   return (Tst_Feedback_t) 0;
  }

/*****************************************************************************/
/**** Check and correct minimum, default and maximum numbers of questions ****/
/*****************************************************************************/

static void Tst_CheckAndCorrectNumbersQst (void)
  {
   /***** Check if minimum is correct *****/
   if (Gbl.Test.Config.Min < 1)
      Gbl.Test.Config.Min = 1;
   else if (Gbl.Test.Config.Min > Tst_MAX_QUESTIONS_PER_EXAM)
      Gbl.Test.Config.Min = Tst_MAX_QUESTIONS_PER_EXAM;

   /***** Check if maximum is correct *****/
   if (Gbl.Test.Config.Max < 1)
      Gbl.Test.Config.Max = 1;
   else if (Gbl.Test.Config.Max > Tst_MAX_QUESTIONS_PER_EXAM)
      Gbl.Test.Config.Max = Tst_MAX_QUESTIONS_PER_EXAM;

   /***** Check if minimum is lower than maximum *****/
   if (Gbl.Test.Config.Min > Gbl.Test.Config.Max)
      Gbl.Test.Config.Min = Gbl.Test.Config.Max;

   /***** Check if default is correct *****/
   if (Gbl.Test.Config.Def < Gbl.Test.Config.Min)
      Gbl.Test.Config.Def = Gbl.Test.Config.Min;
   else if (Gbl.Test.Config.Def > Gbl.Test.Config.Max)
      Gbl.Test.Config.Def = Gbl.Test.Config.Max;
  }

/*****************************************************************************/
/***************** Show form for select the types of answers *****************/
/*****************************************************************************/

static void Tst_ShowFormAnswerTypes (unsigned NumCols)
  {
   extern const char *The_ClassForm[The_NUM_THEMES];
   extern const char *Txt_Types_of_answers;
   extern const char *Txt_All_types_of_answers;
   extern const char *Txt_TST_STR_ANSWER_TYPES[Tst_NUM_ANS_TYPES];
   Tst_AnswerType_t AnsType;
   char UnsignedStr[10+1];
   const char *Ptr;

   /***** Label *****/
   fprintf (Gbl.F.Out,"<tr>"
		      "<td class=\"RIGHT_TOP\">"
		      "<label class=\"%s\">"
		      "%s:"
		      "</label>"
                      "</td>",
	    The_ClassForm[Gbl.Prefs.Theme],Txt_Types_of_answers);

   /***** Select all types of answers *****/
   fprintf (Gbl.F.Out,"<td");
   if (NumCols > 1)
      fprintf (Gbl.F.Out," colspan=\"%u\"",NumCols);
   fprintf (Gbl.F.Out," class=\"LEFT_TOP\">"
	              "<table class=\"CELLS_PAD_2\">"
                      "<tr>"
	              "<td class=\"%s LEFT_MIDDLE\">"
                      "<input type=\"checkbox\" name=\"AllAnsTypes\" value=\"Y\"",
            The_ClassForm[Gbl.Prefs.Theme]);
   if (Gbl.Test.AllAnsTypes)
      fprintf (Gbl.F.Out," checked=\"checked\"");
   fprintf (Gbl.F.Out," onclick=\"togglecheckChildren(this,'AnswerType')\" />"
                      " %s"
                      "</td>"
                      "</tr>",
            Txt_All_types_of_answers);

   /***** Type of answer *****/
   for (AnsType = (Tst_AnswerType_t) 0;
	AnsType < Tst_NUM_ANS_TYPES;
	AnsType++)
     {
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"DAT LEFT_MIDDLE\">"
                         "<input type=\"checkbox\" name=\"AnswerType\" value=\"%u\"",
               (unsigned) AnsType);
      Ptr = Gbl.Test.ListAnsTypes;
      while (*Ptr)
        {
         Par_GetNextStrUntilSeparParamMult (&Ptr,UnsignedStr,10);
         if (Tst_ConvertFromUnsignedStrToAnsTyp (UnsignedStr) == AnsType)
            fprintf (Gbl.F.Out," checked=\"checked\"");
        }
      fprintf (Gbl.F.Out," onclick=\"checkParent(this,'AllAnsTypes')\" />"
	                 " %s"
	                 "</td>"
	                 "</tr>",
               Txt_TST_STR_ANSWER_TYPES[AnsType]);
     }

   fprintf (Gbl.F.Out,"</table>"
	              "</td>"
	              "</tr>");
  }

/*****************************************************************************/
/***************** List several test questions for edition *******************/
/*****************************************************************************/

void Tst_ListQuestionsToEdit (void)
  {
   MYSQL_RES *mysql_res;
   unsigned long NumRows;

   /***** Get parameters, query the database and list the questions *****/
   if (Tst_GetParamsTst ())							// Get parameters of the form
     {
      if ((NumRows = Tst_GetQuestionsForEdit (&mysql_res)) != 0)		// Query database
        {
	 /***** Buttons for edition *****/
         fprintf (Gbl.F.Out,"<div class=\"CONTEXT_MENU\">");
	 if (Gbl.Test.XML.CreateXML)
            TsI_CreateXML (NumRows,mysql_res);	// Create XML file for exporting questions and put a link to download it
         else
            TsI_PutFormToExportQuestions ();	// Button to export questions
	 Tst_PutFormToConfigure ();		// Put form to go to test configuration
	 fprintf (Gbl.F.Out,"</div>");

         Tst_ListOneOrMoreQuestionsToEdit (NumRows,mysql_res);			// Show the table with the questions
        }

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
   else
      Tst_ShowFormAskEditTsts ();						// Show the form again

   /***** Free memory used by the list of tags *****/
   Tst_FreeTagsList ();
  }

/*****************************************************************************/
/********** Get from the database several test questions for listing *********/
/*****************************************************************************/

#define MAX_LENGTH_QUERY_TEST (16*1024)

static unsigned long Tst_GetQuestionsForEdit (MYSQL_RES **mysql_res)
  {
   extern const char *Txt_No_questions_found_matching_your_search_criteria;
   unsigned long NumRows;
   char Query[MAX_LENGTH_QUERY_TEST+1];
   long LengthQuery;
   unsigned NumItemInList;
   const char *Ptr;
   char TagText[Tst_MAX_BYTES_TAG+1];
   char LongStr[1+10+1];
   char UnsignedStr[10+1];
   Tst_AnswerType_t AnsType;
   char CrsCodStr[1+10+1];

   /***** Select questions *****/
   /* Start query */
   /*
   row[ 0] QstCod
   row[ 1] UNIX_TIMESTAMP(EditTime)
   row[ 2] AnsType
   row[ 3] Shuffle
   row[ 4] Stem
   row[ 5] Feedback
   row[ 6] ImageName
   row[ 7] ImageTitle
   row[ 8] NumHits
   row[ 9] NumHitsNotBlank
   row[10] Score
   */
   sprintf (Query,"SELECT tst_questions.QstCod,"
	          "UNIX_TIMESTAMP(tst_questions.EditTime) AS F,"
                  "tst_questions.AnsType,tst_questions.Shuffle,"
                  "tst_questions.Stem,tst_questions.Feedback,"
                  "tst_questions.ImageName,tst_questions.ImageTitle,"
                  "tst_questions.NumHits,tst_questions.NumHitsNotBlank,"
                  "tst_questions.Score"
                 " FROM tst_questions");
   if (!Gbl.Test.Tags.All)
      strcat (Query,",tst_question_tags,tst_tags");

   strcat (Query," WHERE tst_questions.CrsCod='");
   sprintf (CrsCodStr,"%ld",Gbl.CurrentCrs.Crs.CrsCod);
   strcat (Query,CrsCodStr);
   strcat (Query,"' AND tst_questions.EditTime>=FROM_UNIXTIME('");
   sprintf (LongStr,"%ld",(long) Gbl.DateRange.TimeUTC[0]);
   strcat (Query,LongStr);
   strcat (Query,"') AND tst_questions.EditTime<=FROM_UNIXTIME('");
   sprintf (LongStr,"%ld",(long) Gbl.DateRange.TimeUTC[1]);
   strcat (Query,LongStr);
   strcat (Query,"')");

   /* Add the tags selected */
   if (!Gbl.Test.Tags.All)
     {
      strcat (Query," AND tst_questions.QstCod=tst_question_tags.QstCod"
	            " AND tst_question_tags.TagCod=tst_tags.TagCod"
                    " AND tst_tags.CrsCod='");
      strcat (Query,CrsCodStr);
      strcat (Query,"'");
      LengthQuery = strlen (Query);
      NumItemInList = 0;
      Ptr = Gbl.Test.Tags.List;
      while (*Ptr)
        {
         Par_GetNextStrUntilSeparParamMult (&Ptr,TagText,Tst_MAX_BYTES_TAG);
         LengthQuery = LengthQuery + 35 + strlen (TagText) + 1;
         if (LengthQuery > MAX_LENGTH_QUERY_TEST - 256)
            Lay_ShowErrorAndExit ("Query size exceed.");
         strcat (Query,
                 NumItemInList ? " OR tst_tags.TagTxt='" :
                                 " AND (tst_tags.TagTxt='");
         strcat (Query,TagText);
         strcat (Query,"'");
         NumItemInList++;
        }
      strcat (Query,")");
     }

   /* Add the types of answer selected */
   if (!Gbl.Test.AllAnsTypes)
     {
      LengthQuery = strlen (Query);
      NumItemInList = 0;
      Ptr = Gbl.Test.ListAnsTypes;
      while (*Ptr)
        {
         Par_GetNextStrUntilSeparParamMult (&Ptr,UnsignedStr,Tst_MAX_BYTES_TAG);
	 AnsType = Tst_ConvertFromUnsignedStrToAnsTyp (UnsignedStr);
         LengthQuery = LengthQuery + 35 + strlen (Tst_StrAnswerTypesDB[AnsType]) + 1;
         if (LengthQuery > MAX_LENGTH_QUERY_TEST - 256)
            Lay_ShowErrorAndExit ("Query size exceed.");
         strcat (Query,
                 NumItemInList ? " OR tst_questions.AnsType='" :
                                 " AND (tst_questions.AnsType='");
         strcat (Query,Tst_StrAnswerTypesDB[AnsType]);
         strcat (Query,"'");
         NumItemInList++;
        }
      strcat (Query,")");
     }

   /* End the query */
   strcat (Query," GROUP BY tst_questions.QstCod");

   switch (Gbl.Test.SelectedOrderType)
     {
      case Tst_ORDER_STEM:
         strcat (Query," ORDER BY tst_questions.Stem");
         break;
      case Tst_ORDER_NUM_HITS:
         strcat (Query," ORDER BY tst_questions.NumHits DESC,"
                       "tst_questions.Stem");
         break;
      case Tst_ORDER_AVERAGE_SCORE:
         strcat (Query," ORDER BY tst_questions.Score/tst_questions.NumHits DESC,"
                       "tst_questions.NumHits DESC,"
                       "tst_questions.Stem");
         break;
      case Tst_ORDER_NUM_HITS_NOT_BLANK:
         strcat (Query," ORDER BY tst_questions.NumHitsNotBlank DESC,"
                       "tst_questions.Stem");
         break;
      case Tst_ORDER_AVERAGE_SCORE_NOT_BLANK:
         strcat (Query," ORDER BY tst_questions.Score/tst_questions.NumHitsNotBlank DESC,"
                       "tst_questions.NumHitsNotBlank DESC,"
                       "tst_questions.Stem");
         break;
     }

   /* Make the query */
   NumRows = DB_QuerySELECT (Query,mysql_res,"can not get questions");

   if (NumRows == 0)
      Lay_ShowAlert (Lay_INFO,Txt_No_questions_found_matching_your_search_criteria);

   return NumRows;
  }

/*****************************************************************************/
/********* Get from the database several test questions to list them *********/
/*****************************************************************************/

static unsigned long Tst_GetQuestionsForExam (MYSQL_RES **mysql_res)
  {
   char Query[MAX_LENGTH_QUERY_TEST+1];
   long LengthQuery;
   unsigned NumItemInList;
   const char *Ptr;
   char TagText[Tst_MAX_BYTES_TAG+1];
   char UnsignedStr[10+1];
   Tst_AnswerType_t AnsType;
   char StrNumQsts[10+1];

   /***** Select questions without hidden tags *****/
   /*
   row[ 0] QstCod
   row[ 1] UNIX_TIMESTAMP(EditTime)
   row[ 2] AnsType
   row[ 3] Shuffle
   row[ 4] Stem
   row[ 5] Feedback
   row[ 6] ImageName
   row[ 7] ImageTitle
   row[ 8] NumHits
   row[ 9] NumHitsNotBlank
   row[10] Score
   */
   /* Start query */
   // Reject questions with any tag hidden
   // Select only questions with tags
   // DISTINCTROW is necessary to not repeat questions
   sprintf (Query,"SELECT DISTINCTROW tst_questions.QstCod,"
	          "UNIX_TIMESTAMP(tst_questions.EditTime),"
		  "tst_questions.AnsType,tst_questions.Shuffle,"
		  "tst_questions.Stem,tst_questions.Feedback,"
		  "tst_questions.ImageName,tst_questions.ImageTitle,"
		  "tst_questions.NumHits,tst_questions.NumHitsNotBlank,"
		  "tst_questions.Score"
		  " FROM tst_questions,tst_question_tags,tst_tags"
		  " WHERE tst_questions.CrsCod='%ld'"
		  " AND tst_questions.QstCod NOT IN"
		  " (SELECT tst_question_tags.QstCod"
		  " FROM tst_tags,tst_question_tags"
		  " WHERE tst_tags.CrsCod='%ld' AND tst_tags.TagHidden='Y'"
		  " AND tst_tags.TagCod=tst_question_tags.TagCod)"
		  " AND tst_questions.QstCod=tst_question_tags.QstCod"
		  " AND tst_question_tags.TagCod=tst_tags.TagCod"
		  " AND tst_tags.CrsCod='%ld'",
	    Gbl.CurrentCrs.Crs.CrsCod,
	    Gbl.CurrentCrs.Crs.CrsCod,
	    Gbl.CurrentCrs.Crs.CrsCod);

   if (!Gbl.Test.Tags.All) // User has not selected all the tags
     {
      /* Add selected tags */
      LengthQuery = strlen (Query);
      NumItemInList = 0;
      Ptr = Gbl.Test.Tags.List;
      while (*Ptr)
        {
         Par_GetNextStrUntilSeparParamMult (&Ptr,TagText,Tst_MAX_BYTES_TAG);
         LengthQuery = LengthQuery + 35 + strlen (TagText) + 1;
         if (LengthQuery > MAX_LENGTH_QUERY_TEST - 128)
            Lay_ShowErrorAndExit ("Query size exceed.");
         strcat (Query,
                 NumItemInList ? " OR tst_tags.TagTxt='" :
                                 " AND (tst_tags.TagTxt='");
         strcat (Query,TagText);
         strcat (Query,"'");
         NumItemInList++;
        }
      strcat (Query,")");
     }

   /* Add answer types selected */
   if (!Gbl.Test.AllAnsTypes)
     {
      LengthQuery = strlen (Query);
      NumItemInList = 0;
      Ptr = Gbl.Test.ListAnsTypes;
      while (*Ptr)
        {
         Par_GetNextStrUntilSeparParamMult (&Ptr,UnsignedStr,Tst_MAX_BYTES_TAG);
	 AnsType = Tst_ConvertFromUnsignedStrToAnsTyp (UnsignedStr);
         LengthQuery = LengthQuery + 35 + strlen (Tst_StrAnswerTypesDB[AnsType]) + 1;
         if (LengthQuery > MAX_LENGTH_QUERY_TEST - 128)
            Lay_ShowErrorAndExit ("Query size exceed.");
         strcat (Query,
                 NumItemInList ? " OR tst_questions.AnsType='" :
                                 " AND (tst_questions.AnsType='");
         strcat (Query,Tst_StrAnswerTypesDB[AnsType]);
         strcat (Query,"'");
         NumItemInList++;
        }
      strcat (Query,")");
     }

   /* End query */
   strcat (Query," ORDER BY RAND(NOW()) LIMIT ");
   sprintf (StrNumQsts,"%u",Gbl.Test.NumQsts);
   strcat (Query,StrNumQsts);
/*
   if (Gbl.Usrs.Me.LoggedRole == Rol_SYS_ADM)
      Lay_ShowAlert (Lay_INFO,Query);
*/
   /* Make the query */
   return DB_QuerySELECT (Query,mysql_res,"can not get questions");
  }

/*****************************************************************************/
/*********************** List a test question for edition ********************/
/*****************************************************************************/

static void Tst_ListOneQstToEdit (void)
  {
   MYSQL_RES *mysql_res;

   /***** Query database *****/
   if (Tst_GetOneQuestionByCod (Gbl.Test.QstCod,&mysql_res))
      /***** Show the question ready to edit it *****/
      Tst_ListOneOrMoreQuestionsToEdit (1,mysql_res);
   else
      Lay_ShowErrorAndExit ("Can not get question.");

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*********************** Get data of one test question ***********************/
/*****************************************************************************/
// Return true on success, false on error

static bool Tst_GetOneQuestionByCod (long QstCod,MYSQL_RES **mysql_res)
  {
   char Query[512];

   /***** Get data of a question from database *****/
   /*
   row[ 0] QstCod
   row[ 1] UNIX_TIMESTAMP(EditTime)
   row[ 2] AnsType
   row[ 3] Shuffle
   row[ 4] Stem
   row[ 5] Feedback
   row[ 6] ImageName
   row[ 7] ImageTitle
   row[ 8] NumHits
   row[ 9] NumHitsNotBlank
   row[10] Score
   */
   sprintf (Query,"SELECT QstCod,UNIX_TIMESTAMP(EditTime),"
	          "AnsType,Shuffle,Stem,Feedback,ImageName,ImageTitle,"
	          "NumHits,NumHitsNotBlank,Score"
                  " FROM tst_questions"
                  " WHERE QstCod='%ld'",
            QstCod);
   return (DB_QuerySELECT (Query,mysql_res,"can not get data of a question") == 1);
  }

/*****************************************************************************/
/****************** List for edition one or more test questions **************/
/*****************************************************************************/

static void Tst_ListOneOrMoreQuestionsToEdit (unsigned long NumRows,MYSQL_RES *mysql_res)
  {
   extern const char *Txt_Questions;
   extern const char *Txt_No_INDEX;
   extern const char *Txt_Code;
   extern const char *Txt_Date;
   extern const char *Txt_Tags;
   extern const char *Txt_Type;
   extern const char *Txt_TST_STR_ORDER_FULL[Tst_NUM_TYPES_ORDER_QST];
   extern const char *Txt_TST_STR_ORDER_SHORT[Tst_NUM_TYPES_ORDER_QST];
   extern const char *Txt_TST_STR_ANSWER_TYPES[Tst_NUM_ANS_TYPES];
   extern const char *Txt_Shuffle;
   extern const char *Txt_Edit_question;
   extern const char *Txt_Today;
   Tst_QuestionsOrder_t Order;
   unsigned long NumRow;
   MYSQL_ROW row;
   unsigned UniqueId;
   time_t TimeUTC;
   unsigned long NumHitsThisQst;
   unsigned long NumHitsNotBlankThisQst;
   double TotalScoreThisQst;

   /***** Table start *****/
   Lay_StartRoundFrame (NULL,Txt_Questions,Tst_PutIconToCreateNewTstQst);

   /***** Write the heading *****/
   fprintf (Gbl.F.Out,"<table class=\"FRAME_TABLE_MARGIN CELLS_PAD_2\">"
	              "<tr>"
                      "<th colspan=\"2\"></th>"
                      "<th class=\"CENTER_TOP\">"
                      "%s"
                      "</th>"
                      "<th class=\"CENTER_TOP\">"
                      "%s"
                      "</th>"
                      "<th class=\"CENTER_TOP\">"
                      "%s"
                      "</th>"
                      "<th class=\"LEFT_TOP\">"
                      "%s"
                      "</th>"
                      "<th class=\"CENTER_TOP\">"
                      "%s"
                      "</th>"
                      "<th class=\"CENTER_TOP\">"
                      "%s"
                      "</th>",
            Txt_No_INDEX,
            Txt_Code,
            Txt_Date,
            Txt_Tags,
            Txt_Type,
            Txt_Shuffle);

   /* Stem and answers of question */
   /* Number of times that the question has been answered */
   /* Average score */
   for (Order = (Tst_QuestionsOrder_t) 0;
	Order < (Tst_QuestionsOrder_t) Tst_NUM_TYPES_ORDER_QST;
	Order++)
     {
      fprintf (Gbl.F.Out,"<th class=\"LEFT_TOP\">");
      if (NumRows > 1)
        {
         Act_FormStart (ActLstTstQst);
         Sta_WriteParamsDatesSeeAccesses ();
         Tst_WriteParamEditQst ();
         Par_PutHiddenParamUnsigned ("Order",(unsigned) Order);
         Act_LinkFormSubmit (Txt_TST_STR_ORDER_FULL[Order],"TIT_TBL");
         if (Order == Gbl.Test.SelectedOrderType)
            fprintf (Gbl.F.Out,"<u>");
        }
      fprintf (Gbl.F.Out,"%s",Txt_TST_STR_ORDER_SHORT[Order]);
      if (NumRows > 1)
        {
         if (Order == Gbl.Test.SelectedOrderType)
            fprintf (Gbl.F.Out,"</u>");
         fprintf (Gbl.F.Out,"</a>");
         Act_FormEnd ();
        }
      fprintf (Gbl.F.Out,"</th>");
     }

   fprintf (Gbl.F.Out,"</tr>");

   /***** Write rows *****/
   for (NumRow = 0, UniqueId = 1;
	NumRow < NumRows;
	NumRow++, UniqueId++)
     {
      Gbl.RowEvenOdd = NumRow % 2;

      row = mysql_fetch_row (mysql_res);
      /*
      row[ 0] QstCod
      row[ 1] UNIX_TIMESTAMP(EditTime)
      row[ 2] AnsType
      row[ 3] Shuffle
      row[ 4] Stem
      row[ 5] Feedback
      row[ 6] ImageName
      row[ 7] ImageTitle
      row[ 8] NumHits
      row[ 9] NumHitsNotBlank
      row[10] Score
      */
      /***** Create test question *****/
      Tst_QstConstructor ();

      /* row[0] holds the code of the question */
      if ((Gbl.Test.QstCod = Str_ConvertStrCodToLongCod (row[0])) < 0)
         Lay_ShowErrorAndExit ("Wrong code of question.");

      /* Write icon to remove the question */
      fprintf (Gbl.F.Out,"<tr>"
                         "<td class=\"BT%u\">",Gbl.RowEvenOdd);
      Act_FormStart (ActReqRemTstQst);
      Sta_WriteParamsDatesSeeAccesses ();
      Tst_WriteParamEditQst ();
      Par_PutHiddenParamLong ("QstCod",Gbl.Test.QstCod);
      if (NumRows == 1)
         Par_PutHiddenParamChar ("OnlyThisQst",'Y'); // If there are only one row, don't list again after removing
      Lay_PutIconRemove ();
      Act_FormEnd ();
      fprintf (Gbl.F.Out,"</td>");

      /* Write icon to edit the question */
      fprintf (Gbl.F.Out,"<td class=\"BT%u\">",Gbl.RowEvenOdd);
      Act_FormStart (ActEdiOneTstQst);
      Par_PutHiddenParamLong ("QstCod",Gbl.Test.QstCod);
      fprintf (Gbl.F.Out,"<input type=\"image\" src=\"%s/edit64x64.png\""
	                 " alt=\"%s\" title=\"%s\""
	                 " class=\"ICON20x20\" />",
               Gbl.Prefs.IconsURL,
               Txt_Edit_question,
               Txt_Edit_question);
      Act_FormEnd ();
      fprintf (Gbl.F.Out,"</td>");

      /* Write number of question */
      fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL CENTER_TOP COLOR%u\">"
	                 "%lu&nbsp;"
	                 "</td>",
               Gbl.RowEvenOdd,NumRow + 1);

      /* Write question code */
      fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL CENTER_TOP COLOR%u\">"
	                 "%ld&nbsp;"
	                 "</td>",
               Gbl.RowEvenOdd,Gbl.Test.QstCod);

      /* Write the date (row[1] has the UTC date-time) */
      TimeUTC = Dat_GetUNIXTimeFromStr (row[1]);
      fprintf (Gbl.F.Out,"<td id=\"tst_date_%u\""
	                 " class=\"DAT_SMALL CENTER_TOP COLOR%u\">"
                         "<script type=\"text/javascript\">"
                         "writeLocalDateHMSFromUTC('tst_date_%u',%ld,'<br />','%s');"
                         "</script>"
                         "</td>",
               UniqueId,Gbl.RowEvenOdd,
               UniqueId,(long) TimeUTC,Txt_Today);

      /* Write the question tags */
      fprintf (Gbl.F.Out,"<td class=\"LEFT_TOP COLOR%u\">",
               Gbl.RowEvenOdd);
      Tst_GetAndWriteTagsQst (Gbl.Test.QstCod);
      fprintf (Gbl.F.Out,"</td>");

      /* Write the question type (row[2]) */
      Gbl.Test.AnswerType = Tst_ConvertFromStrAnsTypDBToAnsTyp (row[2]);
      fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL CENTER_TOP COLOR%u\">"
	                 "%s&nbsp;"
	                 "</td>",
	       Gbl.RowEvenOdd,
               Txt_TST_STR_ANSWER_TYPES[Gbl.Test.AnswerType]);

      /* Write if shuffle is enabled (row[3]) */
      fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL CENTER_TOP COLOR%u\">",
	       Gbl.RowEvenOdd);
      if (Gbl.Test.AnswerType == Tst_ANS_UNIQUE_CHOICE ||
          Gbl.Test.AnswerType == Tst_ANS_MULTIPLE_CHOICE)
        {
         Act_FormStart (ActShfTstQst);
         Par_PutHiddenParamLong ("QstCod",Gbl.Test.QstCod);
         Sta_WriteParamsDatesSeeAccesses ();
         Tst_WriteParamEditQst ();
         if (NumRows == 1)
	    Par_PutHiddenParamChar ("OnlyThisQst",'Y'); // If editing only one question, don't edit others
         Par_PutHiddenParamUnsigned ("Order",(unsigned) Gbl.Test.SelectedOrderType);
         fprintf (Gbl.F.Out,"<input type=\"checkbox\" name=\"Shuffle\" value=\"Y\"");
         if (Str_ConvertToUpperLetter (row[3][0]) == 'Y')
            fprintf (Gbl.F.Out," checked=\"checked\"");
         fprintf (Gbl.F.Out," onclick=\"document.getElementById('%s').submit();\" />",
                  Gbl.Form.Id);
         Act_FormEnd ();
        }
      fprintf (Gbl.F.Out,"</td>");

      /* Write the stem (row[4]), the image (row[6], row[7]),
         the feedback (row[5]) and the answers */
      fprintf (Gbl.F.Out,"<td class=\"LEFT_TOP COLOR%u\">",
	       Gbl.RowEvenOdd);
      Tst_WriteQstStem (row[4],"TEST_EDI");
      Img_GetImageNameAndTitleFromRow (row[6],row[7],&Gbl.Test.Image);
      Img_ShowImage (&Gbl.Test.Image,"TEST_IMG_EDIT_LIST_STEM");
      Tst_WriteQstFeedback (row[5],"TEST_EDI_LIGHT");
      Tst_WriteAnswersOfAQstEdit (Gbl.Test.QstCod);
      fprintf (Gbl.F.Out,"</td>");

      /* Get number of hits
         (number of times that the question has been answered,
         including blank answers) (row[8]) */
      if (sscanf (row[8],"%lu",&NumHitsThisQst) != 1)
         Lay_ShowErrorAndExit ("Wrong number of hits to a question.");

      /* Get number of hits not blank
         (number of times that the question has been answered
         with a not blank answer) (row[9]) */
      if (sscanf (row[9],"%lu",&NumHitsNotBlankThisQst) != 1)
         Lay_ShowErrorAndExit ("Wrong number of hits not blank to a question.");

      /* Get the acumulated score of the question (row[10]) */
      setlocale (LC_NUMERIC,"en_US.utf8");	// To get decimal point
      if (sscanf (row[10],"%lf",&TotalScoreThisQst) != 1)
         Lay_ShowErrorAndExit ("Wrong score of a question.");
      setlocale (LC_NUMERIC,"es_ES.utf8");	// Return to spanish system (TODO: this should be internationalized!!!!!!!)

      /* Write number of times this question has been answered */
      fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL CENTER_TOP COLOR%u\">"
	                 "%lu"
	                 "</td>",
               Gbl.RowEvenOdd,NumHitsThisQst);

      /* Write average score */
      fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL CENTER_TOP COLOR%u\">",
               Gbl.RowEvenOdd);
      if (NumHitsThisQst)
         fprintf (Gbl.F.Out,"%.2f",TotalScoreThisQst /
                                   (double) NumHitsThisQst);
      else
         fprintf (Gbl.F.Out,"N.A.");
      fprintf (Gbl.F.Out,"</td>");

      /* Write number of times this question has been answered (not blank) */
      fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL CENTER_TOP COLOR%u\">"
	                 "%lu"
	                 "</td>",
               Gbl.RowEvenOdd,
               NumHitsNotBlankThisQst);

      /* Write average score (not blank) */
      fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL CENTER_TOP COLOR%u\">",
               Gbl.RowEvenOdd);
      if (NumHitsNotBlankThisQst)
         fprintf (Gbl.F.Out,"%.2f",TotalScoreThisQst /
                                   (double) NumHitsNotBlankThisQst);
      else
         fprintf (Gbl.F.Out,"N.A.");
      fprintf (Gbl.F.Out,"</td>"
	                 "</tr>");

      /***** Destroy test question *****/
      Tst_QstDestructor ();
     }

   /***** End table *****/
   fprintf (Gbl.F.Out,"</table>");

   /***** Button to add a new question *****/
   Tst_PutButtonToAddQuestion ();

   /***** End frame *****/
   Lay_EndRoundFrame ();
  }

/*****************************************************************************/
/*********** Write hidden parameters for edition of test questions ***********/
/*****************************************************************************/

void Tst_WriteParamEditQst (void)
  {
   Par_PutHiddenParamChar   ("AllTags",
                             Gbl.Test.Tags.All ? 'Y' :
                        	                'N');
   Par_PutHiddenParamString ("ChkTag",
                             Gbl.Test.Tags.List ? Gbl.Test.Tags.List :
                        	                 "");
   Par_PutHiddenParamChar   ("AllAnsTypes",
                             Gbl.Test.AllAnsTypes ? 'Y' :
                        	                    'N');
   Par_PutHiddenParamString ("AnswerType",Gbl.Test.ListAnsTypes);
  }

/*****************************************************************************/
/*************** Get answers of a test question from database ****************/
/*****************************************************************************/

unsigned Tst_GetAnswersQst (long QstCod,MYSQL_RES **mysql_res,bool Shuffle)
  {
   char Query[512];
   unsigned long NumRows;

   /***** Get answers of a question from database *****/
   sprintf (Query,"SELECT AnsInd,Answer,Feedback,ImageName,ImageTitle,Correct"
	          " FROM tst_answers WHERE QstCod='%ld' ORDER BY %s",
            QstCod,
            Shuffle ? "RAND(NOW())" :
        	      "AnsInd");
   if (!(NumRows = DB_QuerySELECT (Query,mysql_res,"can not get answers of a question")))
      Lay_ShowAlert (Lay_ERROR,"Error when getting answers of a question.");

   return (unsigned) NumRows;
  }

/*****************************************************************************/
/**************** Get and write the answers of a test question ***************/
/*****************************************************************************/

static void Tst_WriteAnswersOfAQstEdit (long QstCod)
  {
   extern const char *Txt_TEST_Correct_answer;
   unsigned NumOpt;
   unsigned i;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   char *Answer;
   char *Feedback;
   size_t LengthAnswer;
   size_t LengthFeedback;
   double FloatNum[2];

   Gbl.Test.Answer.NumOptions = Tst_GetAnswersQst (QstCod,&mysql_res,false);	// Result: AnsInd,Answer,Correct,Feedback
   /*
   row[ 0] AnsInd
   row[ 1] Answer
   row[ 2] Feedback
   row[ 3] ImageName
   row[ 4] ImageTitle
   row[ 5] Correct
   */
   /***** Write the answers *****/
   switch (Gbl.Test.AnswerType)
     {
      case Tst_ANS_INT:
         Tst_CheckIfNumberOfAnswersIsOne ();
         row = mysql_fetch_row (mysql_res);
         fprintf (Gbl.F.Out,"<span class=\"TEST_EDI\">(%ld)</span>",
                  Tst_GetIntAnsFromStr (row[1]));
         break;
      case Tst_ANS_FLOAT:
	 if (Gbl.Test.Answer.NumOptions != 2)
            Lay_ShowErrorAndExit ("Wrong float range.");

         for (i = 0;
              i < 2;
              i++)
           {
            row = mysql_fetch_row (mysql_res);
            FloatNum[i] = Tst_GetFloatAnsFromStr (row[1]);
           }
         fprintf (Gbl.F.Out,"<span class=\"TEST_EDI\">([%lg; %lg])</span>",
                  FloatNum[0],FloatNum[1]);
         break;
      case Tst_ANS_TRUE_FALSE:
         Tst_CheckIfNumberOfAnswersIsOne ();
         row = mysql_fetch_row (mysql_res);
         fprintf (Gbl.F.Out,"<span class=\"TEST_EDI\">(");
         Tst_WriteAnsTF (row[1][0]);
         fprintf (Gbl.F.Out,")</span>");
         break;
      case Tst_ANS_UNIQUE_CHOICE:
      case Tst_ANS_MULTIPLE_CHOICE:
      case Tst_ANS_TEXT:
         fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">");
         for (NumOpt = 0;
              NumOpt < Gbl.Test.Answer.NumOptions;
              NumOpt++)
           {
            row = mysql_fetch_row (mysql_res);

            /* Convert the answer (row[1]), that is in HTML, to rigorous HTML */
            LengthAnswer = strlen (row[1]) * Str_MAX_LENGTH_SPEC_CHAR_HTML;
            if ((Answer = malloc (LengthAnswer+1)) == NULL)
               Lay_ShowErrorAndExit ("Not enough memory to store answer.");
            strncpy (Answer,row[1],LengthAnswer);
            Answer[LengthAnswer] = '\0';
            Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
                              Answer,LengthAnswer,false);

            /* Convert the feedback (row[2]), that is in HTML, to rigorous HTML */
            LengthFeedback = 0;
            Feedback = NULL;
            if (row[2])
               if (row[2][0])
        	 {
		  LengthFeedback = strlen (row[2]) * Str_MAX_LENGTH_SPEC_CHAR_HTML;
		  if ((Feedback = malloc (LengthFeedback+1)) == NULL)
		     Lay_ShowErrorAndExit ("Not enough memory to store feedback.");
		  strncpy (Feedback,row[2],LengthFeedback);
		  Feedback[LengthFeedback] = '\0';
                  Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
                                    Feedback,LengthFeedback,false);
        	 }

            /* Copy image */
	    Img_GetImageNameAndTitleFromRow (row[3],row[4],&Gbl.Test.Answer.Options[NumOpt].Image);

            /* Put an icon that indicates whether the answer is correct or wrong */
            fprintf (Gbl.F.Out,"<tr>"
        	               "<td class=\"BT%u\">",
        	     Gbl.RowEvenOdd);
            if (Str_ConvertToUpperLetter (row[5][0]) == 'Y')
               fprintf (Gbl.F.Out,"<img src=\"%s/ok_on16x16.gif\""
        	                  " alt=\"%s\" title=\"%s\""
        	                  " class=\"ICON20x20\" />",
                        Gbl.Prefs.IconsURL,
                        Txt_TEST_Correct_answer,
                        Txt_TEST_Correct_answer);
            fprintf (Gbl.F.Out,"</td>");

            /* Write the number of option */
            fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL LEFT_TOP\">"
        	               "%c)&nbsp;"
        	               "</td>",
                     'a' + (char) NumOpt);

            /* Write the text of the answer and the image */
            fprintf (Gbl.F.Out,"<td class=\"LEFT_TOP\">"
        	               "<div class=\"TEST_EDI\">"
        	               "%s",
                     Answer);
	    Img_ShowImage (&Gbl.Test.Answer.Options[NumOpt].Image,"TEST_IMG_EDIT_LIST_ANS");
            fprintf (Gbl.F.Out,"</div>");

            /* Write the text of the feedback */
            fprintf (Gbl.F.Out,"<div class=\"TEST_EDI_LIGHT\">");
            if (LengthFeedback)
	       fprintf (Gbl.F.Out,"%s",Feedback);
            fprintf (Gbl.F.Out,"</div>"
        	               "</td>"
        	               "</tr>");

	    /* Free memory allocated for the answer and the feedback */
	    free ((void *) Answer);
	    if (LengthFeedback)
	       free ((void *) Feedback);
           }
         fprintf (Gbl.F.Out,"</table>");
	 break;
      default:
         break;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/**************** Write answers of a question when seeing an exam ************/
/*****************************************************************************/

static void Tst_WriteAnswersOfAQstSeeExam (unsigned NumQst,long QstCod,bool Shuffle)
  {
   /***** Write parameter with question code *****/
   Tst_WriteParamQstCod (NumQst,QstCod);

   /***** Write answer depending on type *****/
   switch (Gbl.Test.AnswerType)
     {
      case Tst_ANS_INT:
         Tst_WriteIntAnsSeeExam (NumQst);
         break;
      case Tst_ANS_FLOAT:
         Tst_WriteFloatAnsSeeExam (NumQst);
         break;
      case Tst_ANS_TRUE_FALSE:
         Tst_WriteFormAnsTF (NumQst);
         break;
      case Tst_ANS_UNIQUE_CHOICE:
      case Tst_ANS_MULTIPLE_CHOICE:
         Tst_WriteChoiceAnsSeeExam (NumQst,QstCod,Shuffle);
         break;
      case Tst_ANS_TEXT:
         Tst_WriteTextAnsSeeExam (NumQst);
         break;
      default:
         break;
     }
  }

/*****************************************************************************/
/************* Write answers of a question when assessing an exam ************/
/*****************************************************************************/

static void Tst_WriteAnswersOfAQstAssessExam (unsigned NumQst,long QstCod,
                                              double *ScoreThisQst,bool *AnswerIsNotBlank)
  {
   MYSQL_RES *mysql_res;

   /***** Get answers of a question from database *****/
   Gbl.Test.Answer.NumOptions = Tst_GetAnswersQst (QstCod,&mysql_res,false);	// Result: AnsInd,Answer,Correct
   /*
   row[ 0] AnsInd
   row[ 1] Answer
   row[ 2] Feedback
   row[ 3] ImageName
   row[ 4] ImageTitle
   row[ 5] Correct
   */
   /***** Write answer depending on type *****/
   switch (Gbl.Test.AnswerType)
     {
      case Tst_ANS_INT:
         Tst_WriteIntAnsAssessExam (NumQst,mysql_res,ScoreThisQst,AnswerIsNotBlank);
         break;
      case Tst_ANS_FLOAT:
         Tst_WriteFloatAnsAssessExam (NumQst,mysql_res,ScoreThisQst,AnswerIsNotBlank);
         break;
      case Tst_ANS_TRUE_FALSE:
         Tst_WriteTFAnsAssessExam (NumQst,mysql_res,ScoreThisQst,AnswerIsNotBlank);
         break;
      case Tst_ANS_UNIQUE_CHOICE:
      case Tst_ANS_MULTIPLE_CHOICE:
         Tst_WriteChoiceAnsAssessExam (NumQst,mysql_res,ScoreThisQst,AnswerIsNotBlank);
         break;
      case Tst_ANS_TEXT:
         Tst_WriteTextAnsAssessExam (NumQst,mysql_res,ScoreThisQst,AnswerIsNotBlank);
         break;
      default:
         break;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************** Write false / true answer when seeing an exam ****************/
/*****************************************************************************/

static void Tst_WriteFormAnsTF (unsigned NumQst)
  {
   extern const char *Txt_TF_QST[2];

   /***** Write selector for the answer *****/
   fprintf (Gbl.F.Out,"<select name=\"Ans%06u\">"
                      "<option value=\"\" selected=\"selected\">&nbsp;</option>"
                      "<option value=\"T\">%s</option>"
                      "<option value=\"F\">%s</option>"
                      "</select>",
            NumQst,
            Txt_TF_QST[0],
            Txt_TF_QST[1]);
  }

/*****************************************************************************/
/************** Write false / true answer when seeing an exam ****************/
/*****************************************************************************/

void Tst_WriteAnsTF (char AnsTF)
  {
   extern const char *Txt_TF_QST[2];

   switch (AnsTF)
     {
      case 'T':		// true
         fprintf (Gbl.F.Out,"%s",Txt_TF_QST[0]);
         break;
      case 'F':		// false
         fprintf (Gbl.F.Out,"%s",Txt_TF_QST[1]);
         break;
      default:		// no answer
         fprintf (Gbl.F.Out,"&nbsp;");
         break;
     }
  }

/*****************************************************************************/
/************* Write false / true answer when assessing an exam **************/
/*****************************************************************************/

static void Tst_WriteTFAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                      double *ScoreThisQst,bool *AnswerIsNotBlank)
  {
   MYSQL_ROW row;
   char AnsTF;
   /*
   row[ 0] AnsInd
   row[ 1] Answer
   row[ 2] Feedback
   row[ 3] ImageName
   row[ 4] ImageTitle
   row[ 5] Correct
   */
   /***** Check if number of rows is correct *****/
   Tst_CheckIfNumberOfAnswersIsOne ();

   /***** Get answer true or false *****/
   row = mysql_fetch_row (mysql_res);

   /***** Compute the mark for this question *****/
   AnsTF = Gbl.Test.StrAnswersOneQst[NumQst][0];
   if (AnsTF == '\0')			// User has omitted the answer (the answer is blank)
     {
      *AnswerIsNotBlank = false;
      *ScoreThisQst = 0.0;
     }
   else
     {
      *AnswerIsNotBlank = true;
      if (AnsTF == row[1][0])		// Correct
         *ScoreThisQst = 1.0;
      else				// Wrong
         *ScoreThisQst = -1.0;
     }

   /***** Header with the title of each column *****/
   fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">"
                      "<tr>");
   Tst_WriteHeadUserCorrect ();
   fprintf (Gbl.F.Out,"</tr>");

   /***** Write the user answer *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"%s CENTER_MIDDLE\">",
            (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
             Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK) ?
            (AnsTF == row[1][0] ? "ANS_OK" :
        	                  "ANS_BAD") :
            "ANS");
   Tst_WriteAnsTF (AnsTF);
   fprintf (Gbl.F.Out,"</td>");

   /***** Write the correct answer *****/
   fprintf (Gbl.F.Out,"<td class=\"ANS CENTER_MIDDLE\">");
   if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
      Tst_WriteAnsTF (row[1][0]);
   else
      fprintf (Gbl.F.Out,"?");
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Write the mark *****/
   if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_RESULT ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
     {
      Tst_WriteScoreStart (2);
      if (AnsTF == '\0')		// If user has omitted the answer
         fprintf (Gbl.F.Out,"ANS\">%.2lf",0.0);
      else if (AnsTF == row[1][0])	// If correct
         fprintf (Gbl.F.Out,"ANS_OK\">%.2lf",1.0);
      else				// If wrong
         fprintf (Gbl.F.Out,"ANS_BAD\">%.2lf",-1.0);
      Tst_WriteScoreEnd ();
     }

   fprintf (Gbl.F.Out,"</table>");
  }

/*****************************************************************************/
/******** Write single or multiple choice answer when seeing an exam *********/
/*****************************************************************************/

static void Tst_WriteChoiceAnsSeeExam (unsigned NumQst,long QstCod,bool Shuffle)
  {
   unsigned NumOpt;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned Index;
   bool ErrorInIndex = false;
   char ParamName[3+6+1];

   /***** Create test question *****/
   Tst_QstConstructor ();
   Gbl.Test.QstCod = QstCod;

   /***** Get answers of a question from database *****/
   Gbl.Test.Answer.NumOptions = Tst_GetAnswersQst (QstCod,&mysql_res,Shuffle);	// Result: AnsInd,Answer,Correct
   /*
   row[ 0] AnsInd
   row[ 1] Answer
   row[ 2] Feedback
   row[ 3] ImageName
   row[ 4] ImageTitle
   row[ 5] Correct
   */

   /***** Start of table *****/
   fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">");

   for (NumOpt = 0;
	NumOpt < Gbl.Test.Answer.NumOptions;
	NumOpt++)
     {
      /***** Get next answer *****/
      row = mysql_fetch_row (mysql_res);

      /***** Allocate memory for text in this choice answer *****/
      if (!Tst_AllocateTextChoiceAnswer (NumOpt))
         Lay_ShowErrorAndExit (Gbl.Message);

      /***** Assign index (row[0]). Index is 0,1,2,3... if no shuffle or 1,3,0,2... (example) if shuffle *****/
      // Index is got from databse query
      if (sscanf (row[0],"%u",&Index) == 1)
        {
         if (Index >= Tst_MAX_OPTIONS_PER_QUESTION)
            ErrorInIndex = true;
        }
      else
         ErrorInIndex = true;
      if (ErrorInIndex)
         Lay_ShowErrorAndExit ("Wrong index of answer when showing an exam.");

      /***** Copy text (row[1]) and convert it, that is in HTML, to rigorous HTML ******/
      strncpy (Gbl.Test.Answer.Options[NumOpt].Text,row[1],Tst_MAX_BYTES_ANSWER_OR_FEEDBACK);
      Gbl.Test.Answer.Options[NumOpt].Text[Tst_MAX_BYTES_ANSWER_OR_FEEDBACK] = '\0';
      Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
                        Gbl.Test.Answer.Options[NumOpt].Text,Tst_MAX_BYTES_ANSWER_OR_FEEDBACK,false);

      /***** Copy image *****/
      Img_GetImageNameAndTitleFromRow (row[3],row[4],&Gbl.Test.Answer.Options[NumOpt].Image);

      /***** Write selectors and letter of this option *****/
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"LEFT_TOP\">");
      sprintf (ParamName,"Ind%06u",NumQst);
      Par_PutHiddenParamUnsigned (ParamName,Index);
      fprintf (Gbl.F.Out,"<input type=\"");
      if (Gbl.Test.AnswerType == Tst_ANS_UNIQUE_CHOICE)
         fprintf (Gbl.F.Out,"radio\" onclick=\"selectUnselectRadio(this,this.form.Ans%06u,%u)\"",
                  NumQst,Gbl.Test.Answer.NumOptions);
      else // Gbl.Test.AnswerType == Tst_ANS_MULTIPLE_CHOICE
         fprintf (Gbl.F.Out,"checkbox\"");
      fprintf (Gbl.F.Out," name=\"Ans%06u\" value=\"%u\" /> </td>",NumQst,Index);
      fprintf (Gbl.F.Out,"<td class=\"TEST LEFT_TOP\">"
	                 "%c)&nbsp;"
	                 "</td>",
               'a' + (char) NumOpt);

      /***** Write the option text *****/
      fprintf (Gbl.F.Out,"<td class=\"TEST_EXA LEFT_TOP\">"
	                 "%s",
               Gbl.Test.Answer.Options[NumOpt].Text);
      Img_ShowImage (&Gbl.Test.Answer.Options[NumOpt].Image,"TEST_IMG_SHOW_ANS");
      fprintf (Gbl.F.Out,"</td>"
	                 "</tr>");
     }

   /***** End of table *****/
   fprintf (Gbl.F.Out,"</table>");

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   /***** Destroy test question *****/
   Tst_QstDestructor ();
  }

/*****************************************************************************/
/******* Write single or multiple choice answer when assessing an exam *******/
/*****************************************************************************/

static void Tst_WriteChoiceAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                          double *ScoreThisQst,bool *AnswerIsNotBlank)
  {
   extern const char *Txt_TEST_User_answer;
   extern const char *Txt_TEST_Correct_answer;
   unsigned NumOpt;
   MYSQL_ROW row;
   char StrOneIndex[10+1];
   const char *Ptr;
   unsigned Indexes[Tst_MAX_OPTIONS_PER_QUESTION];	// Indexes of all answers of this question
   int AnsUsr;
   bool AnswersUsr[Tst_MAX_OPTIONS_PER_QUESTION];
   unsigned NumOptTotInQst = 0;
   unsigned NumOptCorrInQst = 0;
   unsigned NumAnsGood = 0;
   unsigned NumAnsBad = 0;

   /***** Get text and correctness of answers for this question
          from database (one row per answer) *****/
   /*
   row[ 0] AnsInd
   row[ 1] Answer
   row[ 2] Feedback
   row[ 3] ImageName
   row[ 4] ImageTitle
   row[ 5] Correct
   */
   for (NumOpt = 0;
	NumOpt < Gbl.Test.Answer.NumOptions;
	NumOpt++)
     {
      /***** Get next answer *****/
      row = mysql_fetch_row (mysql_res);

      /***** Allocate memory for text in this choice option *****/
      if (!Tst_AllocateTextChoiceAnswer (NumOpt))
         Lay_ShowErrorAndExit (Gbl.Message);

      /***** Copy answer text (row[1]) and convert it,
             that is in HTML, to rigorous HTML ******/
      strncpy (Gbl.Test.Answer.Options[NumOpt].Text,row[1],Tst_MAX_BYTES_ANSWER_OR_FEEDBACK);
      Gbl.Test.Answer.Options[NumOpt].Text[Tst_MAX_BYTES_ANSWER_OR_FEEDBACK] = '\0';
      Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
                        Gbl.Test.Answer.Options[NumOpt].Text,Tst_MAX_BYTES_ANSWER_OR_FEEDBACK,false);

      /***** Copy answer feedback (row[2]) and convert it,
             that is in HTML, to rigorous HTML ******/
      if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
	 if (row[2])
	    if (row[2][0])
	      {
	       strncpy (Gbl.Test.Answer.Options[NumOpt].Feedback,row[2],Tst_MAX_BYTES_ANSWER_OR_FEEDBACK);
	       Gbl.Test.Answer.Options[NumOpt].Feedback[Tst_MAX_BYTES_ANSWER_OR_FEEDBACK] = '\0';
	       Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
	                         Gbl.Test.Answer.Options[NumOpt].Feedback,Tst_MAX_BYTES_ANSWER_OR_FEEDBACK,false);
	      }

      /***** Copy image *****/
      Img_GetImageNameAndTitleFromRow (row[3],row[4],&Gbl.Test.Answer.Options[NumOpt].Image);

      /***** Assign correctness (row[5]) of this answer (this option) *****/
      Gbl.Test.Answer.Options[NumOpt].Correct = (Str_ConvertToUpperLetter (row[5][0]) == 'Y');
     }

   /***** Get indexes for this question from string *****/
   for (NumOpt = 0, Ptr = Gbl.Test.StrIndexesOneQst[NumQst];
	NumOpt < Gbl.Test.Answer.NumOptions;
	NumOpt++)
     {
      Par_GetNextStrUntilSeparParamMult (&Ptr,StrOneIndex,10);
      if (sscanf (StrOneIndex,"%u",&(Indexes[NumOpt])) != 1)
         Lay_ShowErrorAndExit ("Wrong index of answer when assessing an exam.");
     }

   /***** Get the user's answers for this question from string *****/
   for (NumOpt = 0;
	NumOpt < Tst_MAX_OPTIONS_PER_QUESTION;
	NumOpt++)
      AnswersUsr[NumOpt] = false;
   for (NumOpt = 0, Ptr = Gbl.Test.StrAnswersOneQst[NumQst];
	NumOpt < Gbl.Test.Answer.NumOptions;
	NumOpt++)
      if (*Ptr)
        {
         Par_GetNextStrUntilSeparParamMult (&Ptr,StrOneIndex,10);
         if (sscanf (StrOneIndex,"%d",&AnsUsr) != 1)
            Lay_ShowErrorAndExit ("Bad user's answer.");
         if (AnsUsr < 0 || AnsUsr >= Tst_MAX_OPTIONS_PER_QUESTION)
            Lay_ShowErrorAndExit ("Bad user's answer.");
         AnswersUsr[AnsUsr] = true;
        }

   /***** Start of table *****/
   fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">"
	              "<tr>");
   Tst_WriteHeadUserCorrect ();
   fprintf (Gbl.F.Out,"<td></td>"
	              "<td></td>"
	              "</tr>");

   /***** Write answers (one row per answer) *****/
   for (NumOpt = 0;
	NumOpt < Gbl.Test.Answer.NumOptions;
	NumOpt++)
     {
      /* Draw icon depending on user's answer */
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"CENTER_TOP\">");
      if (AnswersUsr[Indexes[NumOpt]] == true)	// This answer has been selected by the user
         fprintf (Gbl.F.Out,"<img src=\"%s/%s16x16.gif\""
                            " alt=\"%s\" title=\"%s\""
                            " class=\"ICON20x20\" />",
                  Gbl.Prefs.IconsURL,
                  (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
                   Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK) ?
                   (Gbl.Test.Answer.Options[Indexes[NumOpt]].Correct ? "ok_green" :
                	                                               "ok_red") :
                   "ok_on",
                  Txt_TEST_User_answer,
                  Txt_TEST_User_answer);
      fprintf (Gbl.F.Out,"</td>");

      /* Draw icon that indicates whether the answer is correct */
      fprintf (Gbl.F.Out,"<td class=\"ANS CENTER_TOP\">");
      if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
          Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
        {
         if (Gbl.Test.Answer.Options[Indexes[NumOpt]].Correct)
            fprintf (Gbl.F.Out,"<img src=\"%s/ok_on16x16.gif\""
        	               " alt=\"%s\" title=\"%s\""
        	               " class=\"ICON20x20\" />",
                     Gbl.Prefs.IconsURL,
                     Txt_TEST_Correct_answer,
                     Txt_TEST_Correct_answer);
        }
      else
         fprintf (Gbl.F.Out,"?");
      fprintf (Gbl.F.Out,"</td>");

      /* Answer letter (a, b, c,...) */
      fprintf (Gbl.F.Out,"<td class=\"TEST LEFT_TOP\">"
	                 "%c)&nbsp;"
	                 "</td>",
               'a' + (char) NumOpt);

      /* Answer text and feedback */
      fprintf (Gbl.F.Out,"<td class=\"LEFT_TOP\">"
	                 "<div class=\"TEST_EXA\">"
	                 "%s",
               Gbl.Test.Answer.Options[Indexes[NumOpt]].Text);
      Img_ShowImage (&Gbl.Test.Answer.Options[Indexes[NumOpt]].Image,"TEST_IMG_SHOW_ANS");
      fprintf (Gbl.F.Out,"</div>");
      if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
	 if (Gbl.Test.Answer.Options[Indexes[NumOpt]].Feedback)
	    if (Gbl.Test.Answer.Options[Indexes[NumOpt]].Feedback[0])
	       fprintf (Gbl.F.Out,"<div class=\"TEST_EXA_LIGHT\">"
				  "%s"
				  "</div>",
			Gbl.Test.Answer.Options[Indexes[NumOpt]].Feedback);
      fprintf (Gbl.F.Out,"</td>"
	                 "</tr>");

      NumOptTotInQst++;
      if (AnswersUsr[Indexes[NumOpt]] == true)	// This answer has been selected by the user
        {
         if (Gbl.Test.Answer.Options[Indexes[NumOpt]].Correct)
            NumAnsGood++;
         else
            NumAnsBad++;
        }
      if (Gbl.Test.Answer.Options[Indexes[NumOpt]].Correct)
         NumOptCorrInQst++;
     }

   /***** The answer is blank? *****/
   *AnswerIsNotBlank = NumAnsGood != 0 || NumAnsBad != 0;

   /***** Compute and write the total score of this question *****/
   if (*AnswerIsNotBlank)
     {
      /* Compute the score */
      if (Gbl.Test.AnswerType == Tst_ANS_UNIQUE_CHOICE)
        {
         if (NumOptTotInQst >= 2)	// It should be 2 options at least
            *ScoreThisQst = (double) NumAnsGood -
                            (double) NumAnsBad / (double) (NumOptTotInQst - 1);
         else			// 0 or 1 options (impossible)
            *ScoreThisQst = (double) NumAnsGood;
        }
      else	// Gbl.Test.AnswerType == Tst_ANS_MULTIPLE_CHOICE
        {
         if (NumOptCorrInQst)	// There are correct options in the question
           {
            if (NumOptCorrInQst < NumOptTotInQst)	// If there are correct options and wrong options (typical case)
               *ScoreThisQst = (double) NumAnsGood / (double) NumOptCorrInQst -
                               (double) NumAnsBad / (double) (NumOptTotInQst-NumOptCorrInQst);
            else					// Si todas the opciones son correctas (caso raro)
               *ScoreThisQst = (double) NumAnsGood / (double) NumOptCorrInQst;
           }
         else
           {
            if (NumOptTotInQst)	// There are options but none is correct (extrange case)
               *ScoreThisQst = - (double) NumAnsBad / (double) NumOptTotInQst;
            else			// There are no options (impossible!)
               *ScoreThisQst = 0.0;
           }
        }
     }
   else	// Answer is blank
      *ScoreThisQst = 0.0;

   /* Write the score */
   if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_RESULT ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
     {
      Tst_WriteScoreStart (4);
      if (*ScoreThisQst == 0.0)
         fprintf (Gbl.F.Out,"ANS");
      else if (*ScoreThisQst > 0.0)
         fprintf (Gbl.F.Out,"ANS_OK");
      else
         fprintf (Gbl.F.Out,"ANS_BAD");
      fprintf (Gbl.F.Out,"\">%.2lf",*ScoreThisQst);
      Tst_WriteScoreEnd ();
     }

   /***** End of table *****/
   fprintf (Gbl.F.Out,"</table>");
  }

/*****************************************************************************/
/******************** Write text answer when seeing an exam ******************/
/*****************************************************************************/

static void Tst_WriteTextAnsSeeExam (unsigned NumQst)
  {
   /***** Write input field for the answer *****/
   fprintf (Gbl.F.Out,"<input type=\"text\" name=\"Ans%06u\""
	              " size=\"40\" maxlength=\"%u\" value=\"\" />",
            NumQst,Tst_MAX_SIZE_ANSWERS_ONE_QST);
  }

/*****************************************************************************/
/***************** Write text answer when assessing an exam ******************/
/*****************************************************************************/

static void Tst_WriteTextAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                        double *ScoreThisQst,bool *AnswerIsNotBlank)
  {
   unsigned NumOpt;
   MYSQL_ROW row;
   char TextAnsUsr[Tst_MAX_SIZE_ANSWERS_ONE_QST],TextAnsOK[Tst_MAX_SIZE_ANSWERS_ONE_QST];
   bool Correct = false;
   /*
   row[ 0] AnsInd
   row[ 1] Answer
   row[ 2] Feedback
   row[ 3] ImageName
   row[ 4] ImageTitle
   row[ 5] Correct
   */
   /***** Get text and correctness of answers for this question from database (one row per answer) *****/
   for (NumOpt = 0;
	NumOpt < Gbl.Test.Answer.NumOptions;
	NumOpt++)
     {
      /***** Get next answer *****/
      row = mysql_fetch_row (mysql_res);

      /***** Allocate memory for text in this choice answer *****/
      if (!Tst_AllocateTextChoiceAnswer (NumOpt))
         Lay_ShowErrorAndExit (Gbl.Message);

      /***** Copy answer text (row[1]) and convert it, that is in HTML, to rigorous HTML ******/
      strncpy (Gbl.Test.Answer.Options[NumOpt].Text,row[1],Tst_MAX_BYTES_ANSWER_OR_FEEDBACK);
      Gbl.Test.Answer.Options[NumOpt].Text[Tst_MAX_BYTES_ANSWER_OR_FEEDBACK] = '\0';
      Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,Gbl.Test.Answer.Options[NumOpt].Text,Tst_MAX_BYTES_ANSWER_OR_FEEDBACK,false);

      /***** Copy answer feedback (row[2]) and convert it, that is in HTML, to rigorous HTML ******/
      if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
	 if (row[2])
	    if (row[2][0])
	      {
	       strncpy (Gbl.Test.Answer.Options[NumOpt].Feedback,row[2],Tst_MAX_BYTES_ANSWER_OR_FEEDBACK);
	       Gbl.Test.Answer.Options[NumOpt].Feedback[Tst_MAX_BYTES_ANSWER_OR_FEEDBACK] = '\0';
	       Str_ChangeFormat (Str_FROM_HTML,Str_TO_RIGOROUS_HTML,
	                         Gbl.Test.Answer.Options[NumOpt].Feedback,Tst_MAX_BYTES_ANSWER_OR_FEEDBACK,false);
	      }

      /***** Assign correctness (row[5]) of this answer (this option) *****/
      Gbl.Test.Answer.Options[NumOpt].Correct = (Str_ConvertToUpperLetter (row[5][0]) == 'Y');
     }

   /***** Header with the title of each column *****/
   fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">"
	              "<tr>");
   Tst_WriteHeadUserCorrect ();
   fprintf (Gbl.F.Out,"</tr>");

   /***** Write the user answer *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"");
   if (Gbl.Test.StrAnswersOneQst[NumQst][0])	// If user has answered the question
     {
      /* Filter the user answer */
      strcpy (TextAnsUsr,Gbl.Test.StrAnswersOneQst[NumQst]);
      Str_ReplaceSeveralSpacesForOne (TextAnsUsr);	// Join several spaces into one in answer
      Str_ConvertToComparable (TextAnsUsr);

      for (NumOpt = 0;
	   NumOpt < Gbl.Test.Answer.NumOptions;
	   NumOpt++)
        {
         /* Filter this correct answer */
         strcpy (TextAnsOK,Gbl.Test.Answer.Options[NumOpt].Text);
         Str_ConvertToComparable (TextAnsOK);

         /* Check is user answer is correct */
         if (!strcoll (TextAnsUsr,TextAnsOK))
           {
            Correct = true;
            break;
           }
        }
      fprintf (Gbl.F.Out,"%s CENTER_TOP\">"
	                 "&nbsp;%s&nbsp;",
               (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
                Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK) ?
                (Correct ? "ANS_OK" :
                           "ANS_BAD") :
                "ANS",
               Gbl.Test.StrAnswersOneQst[NumQst]);
     }
   else						// If user has omitted the answer
      fprintf (Gbl.F.Out,"ANS CENTER_TOP\">"
	                 "&nbsp;");
   fprintf (Gbl.F.Out,"</td>");

   /***** Write the correct answers *****/
   if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
     {
      fprintf (Gbl.F.Out,"<td class=\"CENTER_TOP\">"
                         "<table class=\"CELLS_PAD_2\">");
      for (NumOpt = 0;
	   NumOpt < Gbl.Test.Answer.NumOptions;
	   NumOpt++)
        {
         /* Answer letter (a, b, c,...) */
         fprintf (Gbl.F.Out,"<td class=\"TEST LEFT_TOP\">"
                            "%c)&nbsp;"
                            "</td>",
                  'a' + (char) NumOpt);

         /* Answer text and feedback */
         fprintf (Gbl.F.Out,"<td class=\"LEFT_TOP\">"
                            "<div class=\"ANS\">"
                            "%s"
                            "</div>",
                  Gbl.Test.Answer.Options[NumOpt].Text);
	 if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
	    if (Gbl.Test.Answer.Options[NumOpt].Feedback)
	       if (Gbl.Test.Answer.Options[NumOpt].Feedback[0])
		  fprintf (Gbl.F.Out,"<div class=\"TEST_EXA_LIGHT\">"
				     "%s"
				     "</div>",
			   Gbl.Test.Answer.Options[NumOpt].Feedback);
	 fprintf (Gbl.F.Out,"</td>"
			    "</tr>");
        }
      fprintf (Gbl.F.Out,"</table>");
     }
   else
      fprintf (Gbl.F.Out,"<td class=\"ANS CENTER_TOP\">"
	                 "?");
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Compute the mark *****/
   if (!Gbl.Test.StrAnswersOneQst[NumQst][0])	// If user has omitted the answer
     {
      *AnswerIsNotBlank = false;
      *ScoreThisQst = 0.0;
     }
   else
     {
      *AnswerIsNotBlank = true;
      if (Correct)				// If correct
         *ScoreThisQst = 1.0;
      else					// If wrong
         *ScoreThisQst = 0.0;
     }

   /***** Write the mark *****/
   if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_RESULT ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
     {
      Tst_WriteScoreStart (4);
      if (!Gbl.Test.StrAnswersOneQst[NumQst][0])	// If user has omitted the answer
         fprintf (Gbl.F.Out,"ANS\">%.2lf",0.0);
      else if (Correct)				// If correct
         fprintf (Gbl.F.Out,"ANS_OK\">%.2lf",1.0);
      else					// If wrong
         fprintf (Gbl.F.Out,"ANS_BAD\">%.2lf",0.0);
      Tst_WriteScoreEnd ();
     }

   fprintf (Gbl.F.Out,"</table>");
  }

/*****************************************************************************/
/****************** Write integer answer when seeing an exam *****************/
/*****************************************************************************/

static void Tst_WriteIntAnsSeeExam (unsigned NumQst)
  {
   /***** Write input field for the answer *****/
   fprintf (Gbl.F.Out,"<input type=\"text\" name=\"Ans%06u\""
	              " size=\"11\" maxlength=\"11\" value=\"\" />",
            NumQst);
  }

/*****************************************************************************/
/**************** Write integer answer when assessing an exam ****************/
/*****************************************************************************/

static void Tst_WriteIntAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                       double *ScoreThisQst,bool *AnswerIsNotBlank)
  {
   MYSQL_ROW row;
   long IntAnswerUsr,IntAnswerCorr;
   /*
   row[ 0] AnsInd
   row[ 1] Answer
   row[ 2] Feedback
   row[ 3] ImageName
   row[ 4] ImageTitle
   row[ 5] Correct
   */
   /***** Check if number of rows is correct *****/
   Tst_CheckIfNumberOfAnswersIsOne ();

   /***** Get the numerical value of the correct answer *****/
   row = mysql_fetch_row (mysql_res);
   if (sscanf (row[1],"%ld",&IntAnswerCorr) != 1)
      Lay_ShowErrorAndExit ("Wrong integer answer.");

   /***** Header with the title of each column *****/
   fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">"
	              "<tr>");
   Tst_WriteHeadUserCorrect ();
   fprintf (Gbl.F.Out,"</tr>");

   /***** Write the user answer *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"");
   if (Gbl.Test.StrAnswersOneQst[NumQst][0])		// If user has answered the question
     {
      if (sscanf (Gbl.Test.StrAnswersOneQst[NumQst],"%ld",&IntAnswerUsr) == 1)
         fprintf (Gbl.F.Out,"%s CENTER_MIDDLE\">"
                            "&nbsp;%ld&nbsp;",
                  (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
                   Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK) ?
                   (IntAnswerUsr == IntAnswerCorr ? "ANS_OK" :
                	                            "ANS_BAD") :
                   "ANS",
                  IntAnswerUsr);
      else
        {
         Gbl.Test.StrAnswersOneQst[NumQst][0] = '\0';
         fprintf (Gbl.F.Out,"ANS CENTER_MIDDLE\">"
                            "?");
        }
     }
   else							// If user has omitted the answer
      fprintf (Gbl.F.Out,"ANS CENTER_MIDDLE\">&nbsp;");
   fprintf (Gbl.F.Out,"</td>");

   /***** Write the correct answer *****/
   fprintf (Gbl.F.Out,"<td class=\"ANS CENTER_MIDDLE\">");
   if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
      fprintf (Gbl.F.Out,"&nbsp;%ld&nbsp;",IntAnswerCorr);
   else
      fprintf (Gbl.F.Out,"?");
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Compute the score *****/
   if (!Gbl.Test.StrAnswersOneQst[NumQst][0])	// If user has omitted the answer
     {
      *AnswerIsNotBlank = false;
      *ScoreThisQst = 0.0;
     }
   else
     {
      *AnswerIsNotBlank = true;
      if (IntAnswerUsr == IntAnswerCorr)	// If correct
         *ScoreThisQst = 1.0;
      else					// If wrong
         *ScoreThisQst = 0.0;
     }

   /***** Write the score *****/
   if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_RESULT ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
     {
      Tst_WriteScoreStart (2);
      if (!Gbl.Test.StrAnswersOneQst[NumQst][0])	// If user has omitted the answer
         fprintf (Gbl.F.Out,"ANS\">%.2lf",0.0);
      else if (IntAnswerUsr == IntAnswerCorr)	// If correct
         fprintf (Gbl.F.Out,"ANS_OK\">%.2lf",1.0);
      else					// If wrong
         fprintf (Gbl.F.Out,"ANS_BAD\">%.2lf",0.0);
      Tst_WriteScoreEnd ();
     }

   fprintf (Gbl.F.Out,"</table>");
  }

/*****************************************************************************/
/****************** Write float answer when seeing an exam *******************/
/*****************************************************************************/

static void Tst_WriteFloatAnsSeeExam (unsigned NumQst)
  {
   /***** Write input field for the answer *****/
   fprintf (Gbl.F.Out,"<input type=\"text\" name=\"Ans%06u\""
	              " size=\"11\" maxlength=\"%u\" value=\"\" />",
            NumQst,Tst_MAX_BYTES_FLOAT_ANSWER);
  }

/*****************************************************************************/
/***************** Write float answer when assessing an exam *****************/
/*****************************************************************************/

static void Tst_WriteFloatAnsAssessExam (unsigned NumQst,MYSQL_RES *mysql_res,
                                         double *ScoreThisQst,bool *AnswerIsNotBlank)
  {
   MYSQL_ROW row;
   unsigned i;
   double FloatAnsUsr = 0.0,Tmp;
   double FloatAnsCorr[2];
   /*
   row[ 0] AnsInd
   row[ 1] Answer
   row[ 2] Feedback
   row[ 3] ImageName
   row[ 4] ImageTitle
   row[ 5] Correct
   */
   /***** Check if number of rows is correct *****/
   if (Gbl.Test.Answer.NumOptions != 2)
      Lay_ShowErrorAndExit ("Wrong float range.");

   /***** Get the numerical value of the minimum and maximum correct answers *****/
   for (i = 0;
	i < 2;
	i++)
     {
      row = mysql_fetch_row (mysql_res);
      FloatAnsCorr[i] = Tst_GetFloatAnsFromStr (row[1]);
     }
   if (FloatAnsCorr[0] > FloatAnsCorr[1]) 	// The maximum and the minimum are swapped
    {
      /* Swap maximum and minimum */
      Tmp = FloatAnsCorr[0];
      FloatAnsCorr[0] = FloatAnsCorr[1];
      FloatAnsCorr[1] = Tmp;
     }

   /***** Header with the title of each column *****/
   fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">"
	              "<tr>");
   Tst_WriteHeadUserCorrect ();
   fprintf (Gbl.F.Out,"</tr>");

   /***** Write the user answer *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"");
   if (Gbl.Test.StrAnswersOneQst[NumQst][0])	// If user has answered the question
     {
      FloatAnsUsr = Tst_GetFloatAnsFromStr (Gbl.Test.StrAnswersOneQst[NumQst]);
      if (Gbl.Test.StrAnswersOneQst[NumQst][0])	// It's a correct floating point number
         fprintf (Gbl.F.Out,"%s CENTER_MIDDLE\">&nbsp;%lg&nbsp;",
                  (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
                   Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK) ?
                   ((FloatAnsUsr >= FloatAnsCorr[0] &&
                     FloatAnsUsr <= FloatAnsCorr[1]) ? "ANS_OK" :
                	                               "ANS_BAD") :
                   "ANS",
                  FloatAnsUsr);
      else				// Not a floating point number
         fprintf (Gbl.F.Out,"ANS CENTER_MIDDLE\">?");
     }
   else					// If user has omitted the answer
      fprintf (Gbl.F.Out,"ANS CENTER_MIDDLE\">&nbsp;");
   fprintf (Gbl.F.Out,"</td>");

   /***** Write the correct answer *****/
   fprintf (Gbl.F.Out,"<td class=\"ANS CENTER_MIDDLE\">");
   if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
      fprintf (Gbl.F.Out,"&nbsp;[%lg; %lg]&nbsp;",FloatAnsCorr[0],FloatAnsCorr[1]);
   else
      fprintf (Gbl.F.Out,"?");
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Compute mark *****/
   if (!Gbl.Test.StrAnswersOneQst[NumQst][0])	// If user has omitted the answer
     {
      *AnswerIsNotBlank = false;
      *ScoreThisQst = 0.0;
     }
   else
     {
      *AnswerIsNotBlank = true;
      if (FloatAnsUsr >= FloatAnsCorr[0] &&
          FloatAnsUsr <= FloatAnsCorr[1])	// If correct (inside the interval)
         *ScoreThisQst = 1.0;
      else						// If wrong (outside the interval)
         *ScoreThisQst = 0.0;
     }

   /***** Write mark *****/
   if (Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_RESULT ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_EACH_GOOD_BAD ||
       Gbl.Test.Config.FeedbackType == Tst_FEEDBACK_FULL_FEEDBACK)
     {
      Tst_WriteScoreStart (2);
      if (!Gbl.Test.StrAnswersOneQst[NumQst][0])	// If user has omitted the answer
         fprintf (Gbl.F.Out,"ANS\">%.2lf",0.0);
      else if (FloatAnsUsr >= FloatAnsCorr[0] &&
               FloatAnsUsr <= FloatAnsCorr[1])	// If correct (inside the interval)
         fprintf (Gbl.F.Out,"ANS_OK\">%.2lf",1.0);
      else					// If wrong (outside the interval)
         fprintf (Gbl.F.Out,"ANS_BAD\">%.2lf",0.0);
      Tst_WriteScoreEnd ();
     }

   fprintf (Gbl.F.Out,"</table>");
  }

/*****************************************************************************/
/********* Write head with two columns:                               ********/
/********* one for the user's answer and other for the correct answer ********/
/*****************************************************************************/

static void Tst_WriteHeadUserCorrect (void)
  {
   extern const char *Txt_User;
   extern const char *Txt_TST_Correct_ANSWER;

   fprintf (Gbl.F.Out,"<td class=\"DAT_SMALL CENTER_MIDDLE\">"
	              "&nbsp;%s&nbsp;"
	              "</td>"
                      "<td class=\"DAT_SMALL CENTER_MIDDLE\">"
                      "&nbsp;%s&nbsp;"
                      "</td>",
            Txt_User,Txt_TST_Correct_ANSWER);
  }

/*****************************************************************************/
/*********** Write the start ans the end of the score of an answer ***********/
/*****************************************************************************/

static void Tst_WriteScoreStart (unsigned ColSpan)
  {
   extern const char *Txt_Score;

   fprintf (Gbl.F.Out,"<tr>"
	              "<td colspan=\"%u\" class=\"DAT_SMALL LEFT_MIDDLE\">"
	              "%s: <span class=\"",
            ColSpan,Txt_Score);
  }

static void Tst_WriteScoreEnd (void)
  {
   fprintf (Gbl.F.Out,"</span>"
	              "</td>"
	              "</tr>");
  }

/*****************************************************************************/
/*************** Write parameter with the code of a question *****************/
/*****************************************************************************/

static void Tst_WriteParamQstCod (unsigned NumQst,long QstCod)
  {
   char ParamName[3+6+1];

   sprintf (ParamName,"Qst%06u",NumQst);
   Par_PutHiddenParamLong (ParamName,QstCod);
  }

/*****************************************************************************/
/********************* Check if number of answers is one *********************/
/*****************************************************************************/

void Tst_CheckIfNumberOfAnswersIsOne (void)
  {
   if (Gbl.Test.Answer.NumOptions != 1)
      Lay_ShowErrorAndExit ("Wrong answer.");
  }

/*****************************************************************************/
/************************* Get tags of a test question ***********************/
/*****************************************************************************/

unsigned long Tst_GetTagsQst (long QstCod,MYSQL_RES **mysql_res)
  {
   char Query[512];

   /***** Get the tags of a question from database *****/
   sprintf (Query,"SELECT tst_tags.TagTxt FROM tst_question_tags,tst_tags"
                  " WHERE tst_question_tags.QstCod='%ld' AND tst_question_tags.TagCod=tst_tags.TagCod AND tst_tags.CrsCod='%ld'"
                  " ORDER BY tst_question_tags.TagInd",
            QstCod,Gbl.CurrentCrs.Crs.CrsCod);
   return DB_QuerySELECT (Query,mysql_res,"can not get the tags of a question");
  }

/*****************************************************************************/
/******************** Get and write tags of a test question ******************/
/*****************************************************************************/

static void Tst_GetAndWriteTagsQst (long QstCod)
  {
   extern const char *Txt_no_tags;
   unsigned long NumRow,NumRows;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   if ((NumRows = Tst_GetTagsQst (QstCod,&mysql_res)))	// Result: TagTxt
     {
      /***** Write the tags *****/
      fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">");
      for (NumRow = 0;
	   NumRow < NumRows;
	   NumRow++)
        {
         row = mysql_fetch_row (mysql_res);
         fprintf (Gbl.F.Out,"<tr>"
                            "<td class=\"DAT_SMALL LEFT_TOP\">"
                            "&nbsp;&#8226;&nbsp;"
                            "</td>"
                            "<td class=\"DAT_SMALL LEFT_TOP\">"
                            "%s"
                            "</td>"
                            "</tr>",
                  row[0]);
        }
      fprintf (Gbl.F.Out,"</table>");
     }
   else
      fprintf (Gbl.F.Out,"<span class=\"DAT_SMALL\">&nbsp;(%s)&nbsp;</span>",
               Txt_no_tags);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************ Get parameters for the selection of test questions *************/
/*****************************************************************************/
// Return true (OK) if all parameters are found, or false (error) if any necessary parameter is not found

static bool Tst_GetParamsTst (void)
  {
   extern const char *Txt_You_must_select_one_ore_more_tags;
   extern const char *Txt_You_must_select_one_ore_more_types_of_answer;
   extern const char *Txt_The_number_of_questions_must_be_in_the_interval_X;
   char YN[1+1];
   bool Error = false;
   char UnsignedStr[10+1];
   unsigned UnsignedNum;

   /***** Tags *****/
   /* Get parameter that indicates whether all tags are selected */
   Par_GetParToText ("AllTags",YN,1);
   Gbl.Test.Tags.All = (Str_ConvertToUpperLetter (YN[0]) == 'Y');

   /* Get the tags */
   if ((Gbl.Test.Tags.List = malloc (Tst_MAX_BYTES_TAGS_LIST+1)) == NULL)
      Lay_ShowErrorAndExit ("Not enough memory to store tags.");
   Par_GetParMultiToText ("ChkTag",Gbl.Test.Tags.List,Tst_MAX_BYTES_TAGS_LIST);

   /* Check number of tags selected */
   if (Tst_CountNumTagsInList () == 0)	// If no tags selected...
     {					// ...write alert
      Lay_ShowAlert (Lay_WARNING,Txt_You_must_select_one_ore_more_tags);
      Error = true;
     }

   /***** Types of answer *****/
   /* Get parameter that indicates if all types of answer are selected */
   Par_GetParToText ("AllAnsTypes",YN,1);
   Gbl.Test.AllAnsTypes = (Str_ConvertToUpperLetter (YN[0]) == 'Y');

   /* Get types of answer */
   Par_GetParMultiToText ("AnswerType",Gbl.Test.ListAnsTypes,Tst_MAX_BYTES_LIST_ANSWER_TYPES);

   /* Check number of types of answer */
   if (Tst_CountNumAnswerTypesInList () == 0)	// If no types of answer selected...
     {						// ...write warning alert
      Lay_ShowAlert (Lay_WARNING,Txt_You_must_select_one_ore_more_types_of_answer);
      Error = true;
     }

   /***** Get other parameters, depending on action *****/
   if (Gbl.Action.Act == ActSeeTst)
     {
      Tst_GetParamNumQst ();
      if (Gbl.Test.NumQsts < Gbl.Test.Config.Min ||
          Gbl.Test.NumQsts > Gbl.Test.Config.Max)
        {
         sprintf (Gbl.Message,Txt_The_number_of_questions_must_be_in_the_interval_X,
                  Gbl.Test.Config.Min,Gbl.Test.Config.Max);
         Lay_ShowAlert (Lay_WARNING,Gbl.Message);
         Error = true;
        }
     }
   else
     {
      /* Get starting and ending dates */
      Dat_GetIniEndDatesFromForm ();

      /* Get ordering criteria */
      Par_GetParMultiToText ("Order",UnsignedStr,10);
      if (sscanf (UnsignedStr,"%u",&UnsignedNum) == 1)
         Gbl.Test.SelectedOrderType = (Tst_QuestionsOrder_t) ((UnsignedNum < Tst_NUM_TYPES_ORDER_QST) ? UnsignedNum :
                                                                                                        0);
      else
         Gbl.Test.SelectedOrderType = (Tst_QuestionsOrder_t) 0;

      /* Get whether we must create the XML file or not */
      Gbl.Test.XML.CreateXML = Tst_GetCreateXMLFromForm ();
     }

   return !Error;
  }

/*****************************************************************************/
/******************** Get parameter with the number of test ******************/
/*****************************************************************************/

static unsigned Tst_GetAndCheckParamNumTst (void)
  {
   char IntStr[1+10+1];
   int IntNum;

   /***** Get number of test exam *****/
   Par_GetParToText ("NumTst",IntStr,10);
   if (!IntStr[0])
      Lay_ShowErrorAndExit ("Number of test is missing.");

   if (sscanf (IntStr,"%d",&IntNum) == 1)
     {
      if (IntNum < (int) 1)
         Lay_ShowErrorAndExit ("Wrong number of test.");
     }
   else
      Lay_ShowErrorAndExit ("Wrong number of test.");

   return (unsigned) IntNum;
  }

/*****************************************************************************/
/***** Get parameter with the number of questions to generate in an test *****/
/*****************************************************************************/

static void Tst_GetParamNumQst (void)
  {
   char UnsignedStr[10+1];
   unsigned UnsignedNum;

   /***** Set default number of questions *****/
   Gbl.Test.NumQsts = Gbl.Test.Config.Def;

   /***** Get number of questions from form *****/
   Par_GetParToText ("NumQst",UnsignedStr,10);
   if (UnsignedStr[0])
      if (sscanf (UnsignedStr,"%u",&UnsignedNum) == 1)
	 Gbl.Test.NumQsts = UnsignedNum;
  }

/*****************************************************************************/
/****************** Get whether to create XML file from form *****************/
/*****************************************************************************/

static bool Tst_GetCreateXMLFromForm (void)
  {
   char YN[1+1];

   Par_GetParToText ("CreateXML",YN,1);
   return (Str_ConvertToUpperLetter (YN[0]) == 'Y');
  }

/*****************************************************************************/
/***************** Count number of tags in the list of tags ******************/
/*****************************************************************************/

static int Tst_CountNumTagsInList (void)
  {
   const char *Ptr;
   int NumTags = 0;
   char TagText[Tst_MAX_BYTES_TAG+1];

   /***** Go over the list Gbl.Test.Tags.List counting the number of tags *****/
   if (Gbl.Test.Tags.List)
     {
      Ptr = Gbl.Test.Tags.List;
      while (*Ptr)
        {
         Par_GetNextStrUntilSeparParamMult (&Ptr,TagText,Tst_MAX_BYTES_TAG);
         NumTags++;
        }
     }
   return NumTags;
  }

/*****************************************************************************/
/**** Count the number of types of answers in the list of types of answers ***/
/*****************************************************************************/

static int Tst_CountNumAnswerTypesInList (void)
  {
   const char *Ptr;
   int NumAnsTypes = 0;
   char UnsignedStr[10+1];

   /***** Go over the list Gbl.Test.ListAnsTypes counting the number of types of answer *****/
   Ptr = Gbl.Test.ListAnsTypes;
   while (*Ptr)
     {
      Par_GetNextStrUntilSeparParamMult (&Ptr,UnsignedStr,10);
      Tst_ConvertFromUnsignedStrToAnsTyp (UnsignedStr);
      NumAnsTypes++;
     }
   return NumAnsTypes;
  }

/*****************************************************************************/
/**************** Free memory allocated for the list of tags *****************/
/*****************************************************************************/

void Tst_FreeTagsList (void)
  {
   if (Gbl.Test.Tags.List)
     {
      free ((void *) Gbl.Test.Tags.List);
      Gbl.Test.Tags.List = NULL;
      Gbl.Test.Tags.Num = 0;
     }
  }

/*****************************************************************************/
/******************** Show form to edit one test question ********************/
/*****************************************************************************/

void Tst_ShowFormEditOneQst (void)
  {
   char Stem[Cns_MAX_BYTES_TEXT+1];
   char Feedback[Cns_MAX_BYTES_TEXT+1];

   /***** Create test question *****/
   Tst_QstConstructor ();
   Gbl.Test.QstCod = Tst_GetQstCod ();
   Stem[0] = Feedback[0] = '\0';
   if (Gbl.Test.QstCod > 0)	// If question already exists in the database
      Tst_GetQstDataFromDB (Stem,Feedback);

   /***** Put form to edit question *****/
   Tst_PutFormEditOneQst (Stem,Feedback);

   /***** Destroy test question *****/
   Tst_QstDestructor ();
  }

/*****************************************************************************/
/******************** Show form to edit one test question ********************/
/*****************************************************************************/

// This function may be called from three places:
// 1. Pressing edition of question in the menu
// 2. Pressing button to edit question in a listing of existing questions
// 3. From the action associated to reception of a question, on error in the parameters received from the form

static void Tst_PutFormEditOneQst (char *Stem,char *Feedback)
  {
   extern const char *The_ClassForm[The_NUM_THEMES];
   extern const char *Txt_Question_code_X;
   extern const char *Txt_New_question;
   extern const char *Txt_Tags;
   extern const char *Txt_new_tag;
   extern const char *Txt_Stem;
   extern const char *Txt_Feedback;
   extern const char *Txt_optional;
   extern const char *Txt_Type;
   extern const char *Txt_TST_STR_ANSWER_TYPES[Tst_NUM_ANS_TYPES];
   extern const char *Txt_Answers;
   extern const char *Txt_Integer_number;
   extern const char *Txt_Real_number_between_A_and_B_1;
   extern const char *Txt_Real_number_between_A_and_B_2;
   extern const char *Txt_TF_QST[2];
   extern const char *Txt_Shuffle;
   extern const char *Txt_Save;
   extern const char *Txt_Create_question;
   char Title[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRows;
   unsigned long NumRow;
   unsigned NumOpt;
   Tst_AnswerType_t AnsType;
   unsigned NumTag;
   bool TagNotFound;
   bool OptionsDisabled;
   bool AnswerHasContent;
   char ParamAction[32];
   char ParamFile[32];
   char ParamTitle[32];

   /***** Start frame *****/
   if (Gbl.Test.QstCod > 0)	// The question already has assigned a code
     {
      sprintf (Title,Txt_Question_code_X,Gbl.Test.QstCod);
      Lay_StartRoundFrame (NULL,Title,Tst_PutIconToRemoveOneQst);
     }
   else
      Lay_StartRoundFrame (NULL,Txt_New_question,NULL);

   /***** Start form *****/
   Act_FormStart (ActRcvTstQst);
   if (Gbl.Test.QstCod > 0)	// The question already has assigned a code
      Par_PutHiddenParamLong ("QstCod",Gbl.Test.QstCod);

   /***** Start table *****/
   fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\">");

   /***** Help for text editor *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td colspan=\"2\">");
   Lay_HelpPlainEditor ();
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Get tags already existing for questions in current course *****/
   NumRows = Tst_GetAllTagsFromCurrentCrs (&mysql_res);

   /***** Write the tags *****/
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"RIGHT_TOP\">"
	              "<label class=\"%s\">"
                      "%s:"
                      "</label>"
                      "</td>"
                      "<td class=\"LEFT_TOP\">"
                      "<table class=\"CELLS_PAD_2\">",
            The_ClassForm[Gbl.Prefs.Theme],Txt_Tags);
   for (NumTag = 0;
	NumTag < Tst_MAX_TAGS_PER_QUESTION;
	NumTag++)
     {
      fprintf (Gbl.F.Out,"<tr>");

      /***** Write the tags already existing in a selector *****/
      fprintf (Gbl.F.Out,"<td class=\"LEFT_MIDDLE\">"
	                 "<select id=\"SelDesc%u\" name=\"SelDesc%u\""
	                 " class=\"TAG_SEL\" onchange=\"changeTxtTag('%u')\">",
               NumTag,NumTag,NumTag);
      fprintf (Gbl.F.Out,"<option value=\"\">&nbsp;</option>");
      mysql_data_seek (mysql_res, 0);
      TagNotFound = true;
      for (NumRow = 1;
	   NumRow <= NumRows;
	   NumRow++)
        {
         row = mysql_fetch_row (mysql_res);

         fprintf (Gbl.F.Out,"<option value=\"%s\"",row[1]);
         if (!strcasecmp (Gbl.Test.Tags.Txt[NumTag],row[1]))
           {
  	    fprintf (Gbl.F.Out," selected=\"selected\"");
            TagNotFound = false;
           }
         fprintf (Gbl.F.Out,">%s</option>",row[1]);
        }
      /* If it's a new tag received from the form */
      if (TagNotFound && Gbl.Test.Tags.Txt[NumTag][0])
         fprintf (Gbl.F.Out,"<option value=\"%s\" selected=\"selected\">%s</option>",
                  Gbl.Test.Tags.Txt[NumTag],Gbl.Test.Tags.Txt[NumTag]);
      fprintf (Gbl.F.Out,"<option value=\"\">[%s]</option>"
	                 "</select>"
	                 "</td>",
               Txt_new_tag);

      /***** Input of a new tag *****/
      fprintf (Gbl.F.Out,"<td class=\"RIGHT_MIDDLE\">"
                         "<input type=\"text\" id=\"TagTxt%u\" name=\"TagTxt%u\""
                         " class=\"TAG_TXT\" maxlength=\"%u\" value=\"%s\""
                         " onchange=\"changeSelTag('%u')\" />"
                         "</td>",
	       NumTag,NumTag,Tst_MAX_TAG_LENGTH,Gbl.Test.Tags.Txt[NumTag],NumTag);

      fprintf (Gbl.F.Out,"</tr>");
     }
   fprintf (Gbl.F.Out,"</table>"
	              "</td>"
	              "</tr>");

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);

   /***** Stem and image *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"RIGHT_TOP\">"
	              "<label class=\"%s\">"
	              "%s:"
                      "</label>"
	              "</td>"
                      "<td class=\"LEFT_TOP\">"
                      "<textarea name=\"Stem\" class=\"STEM\" rows=\"5\">"
                      "%s"
                      "</textarea><br />",
            The_ClassForm[Gbl.Prefs.Theme],
            Txt_Stem,
            Stem);
   Tst_PutFormToEditQstImage (&Gbl.Test.Image,"TEST_IMG_EDIT_ONE_STEM",
                              "STEM",	// Title / attribution
                              "ImgAct","FilImg","TitImg",false);

   /***** Feedback *****/
   fprintf (Gbl.F.Out,"<label class=\"%s\">"
	              "%s (%s):"
                      "</label><br />"
                      "<textarea name=\"Feedback\" class=\"STEM\" rows=\"2\">",
            The_ClassForm[Gbl.Prefs.Theme],
            Txt_Feedback,Txt_optional);
   if (Feedback)
      if (Feedback[0])
	 fprintf (Gbl.F.Out,"%s",Feedback);
   fprintf (Gbl.F.Out,"</textarea>"
                      "</td>"
                      "</tr>");

   /***** Type of answer *****/
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"RIGHT_TOP\">"
	              "<label class=\"%s\">"
	              "%s:"
                      "</label>"
	              "</td>"
                      "<td class=\"%s LEFT_TOP\">",
            The_ClassForm[Gbl.Prefs.Theme],
            Txt_Type,
            The_ClassForm[Gbl.Prefs.Theme]);
   for (AnsType = (Tst_AnswerType_t) 0;
	AnsType < Tst_NUM_ANS_TYPES;
	AnsType++)
     {
      fprintf (Gbl.F.Out,"<input type=\"radio\" name=\"AnswerType\" value=\"%u\"",
               (unsigned) AnsType);
      if (AnsType == Gbl.Test.AnswerType)
         fprintf (Gbl.F.Out," checked=\"checked\"");
      fprintf (Gbl.F.Out," onclick=\"enableDisableAns(this.form)\" />%s&nbsp;<br />",
               Txt_TST_STR_ANSWER_TYPES[AnsType]);
     }
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Answers *****/
   /* Integer answer */
   fprintf (Gbl.F.Out,"<tr>"
                      "<td class=\"RIGHT_TOP\">"
	              "<label class=\"%s\">"
                      "%s:"
                      "</label>"
                      "</td>"
                      "<td class=\"%s LEFT_TOP\">"
                      "%s: "
                      "<input type=\"text\" name=\"AnsInt\""
                      " size=\"11\" maxlength=\"11\" value=\"%ld\"",
            The_ClassForm[Gbl.Prefs.Theme],Txt_Answers,
            The_ClassForm[Gbl.Prefs.Theme],Txt_Integer_number,
            Gbl.Test.Answer.Integer);
   if (Gbl.Test.AnswerType != Tst_ANS_INT)
      fprintf (Gbl.F.Out," disabled=\"disabled\"");
   fprintf (Gbl.F.Out," /></td>"
	              "</tr>");

   /* Floating point answer */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td></td>"
                      "<td class=\"%s LEFT_TOP\">"
                      "%s "
                      "<input type=\"text\" name=\"AnsFloatMin\""
                      " size=\"11\" maxlength=\"%u\" value=\"%lg\"",
            The_ClassForm[Gbl.Prefs.Theme],Txt_Real_number_between_A_and_B_1,
            Tst_MAX_BYTES_FLOAT_ANSWER,Gbl.Test.Answer.FloatingPoint[0]);
   if (Gbl.Test.AnswerType != Tst_ANS_FLOAT)
      fprintf (Gbl.F.Out," disabled=\"disabled\"");
   fprintf (Gbl.F.Out," /> %s "
	              "<input type=\"text\" name=\"AnsFloatMax\""
	              " size=\"11\" maxlength=\"%u\" value=\"%lg\"",
            Txt_Real_number_between_A_and_B_2,
            Tst_MAX_BYTES_FLOAT_ANSWER,Gbl.Test.Answer.FloatingPoint[1]);
   if (Gbl.Test.AnswerType != Tst_ANS_FLOAT)
      fprintf (Gbl.F.Out," disabled=\"disabled\"");
   fprintf (Gbl.F.Out," /></td>"
	              "</tr>");

   /* T/F answer */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td></td>"
                      "<td class=\"LEFT_TOP\">"
                      "<table class=\"CELLS_PAD_2\">"
                      "<tr>"
	              "<td class=\"%s LEFT_TOP\">"
                      "<input type=\"radio\" name=\"AnsTF\" value=\"T\"",
            The_ClassForm[Gbl.Prefs.Theme]);
   if (Gbl.Test.Answer.TF == 'T')
      fprintf (Gbl.F.Out," checked=\"checked\"");
   if (Gbl.Test.AnswerType != Tst_ANS_TRUE_FALSE)
      fprintf (Gbl.F.Out," disabled=\"disabled\"");
   fprintf (Gbl.F.Out," />%s&nbsp;<input type=\"radio\" name=\"AnsTF\" value=\"F\"",
	    Txt_TF_QST[0]);
   if (Gbl.Test.Answer.TF == 'F')
      fprintf (Gbl.F.Out," checked=\"checked\"");
   if (Gbl.Test.AnswerType != Tst_ANS_TRUE_FALSE)
      fprintf (Gbl.F.Out," disabled=\"disabled\"");
   fprintf (Gbl.F.Out," />%s</td>"
	              "</tr>"
	              "</table>"
	              "</td>"
	              "</tr>",
	    Txt_TF_QST[1]);

   /* Questions can be shuffled? */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td></td>"
                      "<td class=\"%s LEFT_TOP\">"
                      "<input type=\"checkbox\" name=\"Shuffle\" value=\"Y\"",
            The_ClassForm[Gbl.Prefs.Theme]);
   if (Gbl.Test.Shuffle)
      fprintf (Gbl.F.Out," checked=\"checked\"");
   if (Gbl.Test.AnswerType != Tst_ANS_UNIQUE_CHOICE &&
       Gbl.Test.AnswerType != Tst_ANS_MULTIPLE_CHOICE)
      fprintf (Gbl.F.Out," disabled=\"disabled\"");
   fprintf (Gbl.F.Out," />%s</td>"
	              "</tr>",
            Txt_Shuffle);

   /* Simple or multiple choice answers */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td></td>"
	              "<td class=\"LEFT_TOP\">"
	              "<table class=\"CELLS_PAD_2\">");
   OptionsDisabled = Gbl.Test.AnswerType != Tst_ANS_UNIQUE_CHOICE &&
                     Gbl.Test.AnswerType != Tst_ANS_MULTIPLE_CHOICE &&
	             Gbl.Test.AnswerType != Tst_ANS_TEXT;
   for (NumOpt = 0;
	NumOpt < Tst_MAX_OPTIONS_PER_QUESTION;
	NumOpt++)
     {
      Gbl.RowEvenOdd = NumOpt % 2;

      /***** Left column: selectors and letter of the answer *****/
      fprintf (Gbl.F.Out,"<tr>"
	                 "<td class=\"%s TEST_EDI_ANS_LEFT_COL COLOR%u\">"
	                 "<input type=\"radio\" name=\"AnsUni\" value=\"%u\"",
	       The_ClassForm[Gbl.Prefs.Theme],Gbl.RowEvenOdd,
	       NumOpt);
      if (Gbl.Test.AnswerType != Tst_ANS_UNIQUE_CHOICE)
         fprintf (Gbl.F.Out," disabled=\"disabled\"");
      if (Gbl.Test.Answer.Options[NumOpt].Correct)
         fprintf (Gbl.F.Out," checked=\"checked\"");
      fprintf (Gbl.F.Out," />"
	                 "<input type=\"checkbox\" name=\"AnsMulti\" value=\"%u\"",
	       NumOpt);
      if (Gbl.Test.AnswerType != Tst_ANS_MULTIPLE_CHOICE)
         fprintf (Gbl.F.Out," disabled=\"disabled\"");
      if (Gbl.Test.Answer.Options[NumOpt].Correct)
         fprintf (Gbl.F.Out," checked=\"checked\"");
      fprintf (Gbl.F.Out," />"
	                 "&nbsp;"
                         "<a href=\"\" class=\"%s\""
                         " onclick=\"toggleDisplay('answer_%u');return false;\" />"
	                 "%c)"
	                 "</a>"
                         "</td>",
               The_ClassForm[Gbl.Prefs.Theme],
               NumOpt,
               'a' + (char) NumOpt);

      /***** Right column: content of the answer *****/
      AnswerHasContent = false;
      if (Gbl.Test.Answer.Options[NumOpt].Text)
	 if (Gbl.Test.Answer.Options[NumOpt].Text[0])
	    AnswerHasContent = true;

      fprintf (Gbl.F.Out,"<td class=\"TEST_EDI_ANS_RIGHT_COL COLOR%u\">"
	                 "<div id=\"answer_%u\"",
	       Gbl.RowEvenOdd,
	       NumOpt);
      if (!AnswerHasContent)	// Answer does not have content
	 fprintf (Gbl.F.Out," style=\"display:none;\"");	// Hide column
      fprintf (Gbl.F.Out,">");

      /* Answer text */
      fprintf (Gbl.F.Out,"<textarea name=\"AnsStr%u\""
	                 " class=\"ANS_STR\" rows=\"5\"",NumOpt);
      if (OptionsDisabled)
         fprintf (Gbl.F.Out," disabled=\"disabled\"");
      fprintf (Gbl.F.Out,">");
      if (AnswerHasContent)
         fprintf (Gbl.F.Out,"%s",Gbl.Test.Answer.Options[NumOpt].Text);
      fprintf (Gbl.F.Out,"</textarea>");

      /* Image */
      sprintf (ParamAction,"ImgAct%u",NumOpt);
      sprintf (ParamFile  ,"FilImg%u",NumOpt);
      sprintf (ParamTitle ,"TitImg%u",NumOpt);
      Tst_PutFormToEditQstImage (&Gbl.Test.Answer.Options[NumOpt].Image,
                                 "TEST_IMG_EDIT_ONE_ANS",
                                 "ANS_STR",	// Title / attribution
                                 ParamAction,ParamFile,ParamTitle,
                                 OptionsDisabled);

      /* Feedback */
      fprintf (Gbl.F.Out,"<div class=\"%s\">%s (%s):</div>"
	                 "<textarea name=\"FbStr%u\" class=\"ANS_STR\" rows=\"2\"",
	       The_ClassForm[Gbl.Prefs.Theme],Txt_Feedback,Txt_optional,
	       NumOpt);
      if (OptionsDisabled)
         fprintf (Gbl.F.Out," disabled=\"disabled\"");
      fprintf (Gbl.F.Out,">");
      if (Gbl.Test.Answer.Options[NumOpt].Feedback)
         if (Gbl.Test.Answer.Options[NumOpt].Feedback[0])
            fprintf (Gbl.F.Out,"%s",Gbl.Test.Answer.Options[NumOpt].Feedback);
      fprintf (Gbl.F.Out,"</textarea>");

      /* End of right column */
      fprintf (Gbl.F.Out,"</div>"
                         "</td>"
	                 "</tr>");
     }
   fprintf (Gbl.F.Out,"</table>"
	              "</td>"
	              "</tr>");

   /***** End table *****/
   fprintf (Gbl.F.Out,"</table>");

   /***** Send button *****/
   if (Gbl.Test.QstCod > 0)	// The question already has assigned a code
      Lay_PutConfirmButton (Txt_Save);
   else
      Lay_PutCreateButton (Txt_Create_question);

   /***** End form *****/
   Act_FormEnd ();

   /***** End frame *****/
   Lay_EndRoundFrame ();
  }

/*****************************************************************************/
/********************* Initialize a new question to zero *********************/
/*****************************************************************************/

void Tst_QstConstructor (void)
  {
   unsigned NumOpt;

   Gbl.Test.QstCod = -1L;
   Gbl.Test.Stem.Text = NULL;
   Gbl.Test.Stem.Length = 0;
   Gbl.Test.Feedback.Text = NULL;
   Gbl.Test.Feedback.Length = 0;
   Gbl.Test.Shuffle = false;
   Gbl.Test.AnswerType = Tst_ANS_UNIQUE_CHOICE;
   Gbl.Test.Answer.NumOptions = 0;
   Gbl.Test.Answer.TF = ' ';
   for (NumOpt = 0;
	NumOpt < Tst_MAX_OPTIONS_PER_QUESTION;
	NumOpt++)
     {
      Gbl.Test.Answer.Options[NumOpt].Correct  = false;
      Gbl.Test.Answer.Options[NumOpt].Text     = NULL;
      Gbl.Test.Answer.Options[NumOpt].Feedback = NULL;
     }
   Gbl.Test.Answer.Integer = 0;
   Gbl.Test.Answer.FloatingPoint[0] =
   Gbl.Test.Answer.FloatingPoint[1] = 0.0;

   Tst_InitImagesOfQuestion ();
  }

/*****************************************************************************/
/***************** Free memory allocated for test question *******************/
/*****************************************************************************/

void Tst_QstDestructor (void)
  {
   Tst_FreeTextChoiceAnswers ();
   Tst_FreeImagesOfQuestion ();
  }

/*****************************************************************************/
/******************* Allocate memory for a choice answer *********************/
/*****************************************************************************/

int Tst_AllocateTextChoiceAnswer (unsigned NumOpt)
  {
   Tst_FreeTextChoiceAnswer (NumOpt);

   if ((Gbl.Test.Answer.Options[NumOpt].Text =
	malloc (Tst_MAX_BYTES_ANSWER_OR_FEEDBACK + 1)) == NULL)
     {
      sprintf (Gbl.Message,"Not enough memory to store answer.");
      return 0;
     }
   if ((Gbl.Test.Answer.Options[NumOpt].Feedback =
	malloc (Tst_MAX_BYTES_ANSWER_OR_FEEDBACK + 1)) == NULL)
     {
      sprintf (Gbl.Message,"Not enough memory to store feedback.");
      return 0;
     }

   Gbl.Test.Answer.Options[NumOpt].Text[0] =
   Gbl.Test.Answer.Options[NumOpt].Feedback[0] = '\0';
   return 1;
  }

/*****************************************************************************/
/******************** Free memory of all choice answers **********************/
/*****************************************************************************/

static void Tst_FreeTextChoiceAnswers (void)
  {
   unsigned NumOpt;

   for (NumOpt = 0;
	NumOpt < Tst_MAX_OPTIONS_PER_QUESTION;
	NumOpt++)
      Tst_FreeTextChoiceAnswer (NumOpt);
  }

/*****************************************************************************/
/********************** Free memory of a choice answer ***********************/
/*****************************************************************************/

static void Tst_FreeTextChoiceAnswer (unsigned NumOpt)
  {
   if (Gbl.Test.Answer.Options[NumOpt].Text)
     {
      free ((void *) Gbl.Test.Answer.Options[NumOpt].Text);
      Gbl.Test.Answer.Options[NumOpt].Text = NULL;
     }
   if (Gbl.Test.Answer.Options[NumOpt].Feedback)
     {
      free ((void *) Gbl.Test.Answer.Options[NumOpt].Feedback);
      Gbl.Test.Answer.Options[NumOpt].Feedback = NULL;
     }
  }

/*****************************************************************************/
/***************** Initialize images of a question to zero *******************/
/*****************************************************************************/

static void Tst_InitImagesOfQuestion (void)
  {
   unsigned NumOpt;

   Gbl.Test.Image.Action = Img_ACTION_NO_IMAGE;
   Gbl.Test.Image.Status = Img_FILE_NONE;
   Gbl.Test.Image.Name[0] = '\0';
   Gbl.Test.Image.Title = NULL;
   for (NumOpt = 0;
	NumOpt < Tst_MAX_OPTIONS_PER_QUESTION;
	NumOpt++)
     {
      Gbl.Test.Answer.Options[NumOpt].Image.Action = Img_ACTION_NO_IMAGE;
      Gbl.Test.Answer.Options[NumOpt].Image.Status = Img_FILE_NONE;
      Gbl.Test.Answer.Options[NumOpt].Image.Name[0] = '\0';
      Gbl.Test.Answer.Options[NumOpt].Image.Title = NULL;
     }
  }

/*****************************************************************************/
/*********************** Free images of a question ***************************/
/*****************************************************************************/

static void Tst_FreeImagesOfQuestion (void)
  {
   unsigned NumOpt;

   Img_FreeImageTitle (&Gbl.Test.Image);
   for (NumOpt = 0;
	NumOpt < Tst_MAX_OPTIONS_PER_QUESTION;
	NumOpt++)
      Img_FreeImageTitle (&Gbl.Test.Answer.Options[NumOpt].Image);
  }

/*****************************************************************************/
/****************** Get data of a question from database *********************/
/*****************************************************************************/

static void Tst_GetQstDataFromDB (char *Stem,char *Feedback)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRows;
   unsigned long NumRow;
   unsigned NumOpt;

   /***** Get the type of answer and the stem from the database *****/
   /* Get the question from database */
   sprintf (Query,"SELECT AnsType,Shuffle,Stem,Feedback,ImageName,ImageTitle"
		  " FROM tst_questions"
		  " WHERE QstCod='%ld' AND CrsCod='%ld'",
	    Gbl.Test.QstCod,Gbl.CurrentCrs.Crs.CrsCod);
   DB_QuerySELECT (Query,&mysql_res,"can not get a question");

   row = mysql_fetch_row (mysql_res);
   /*
   row[ 0] AnsType
   row[ 1] Shuffle
   row[ 2] Stem
   row[ 3] Feedback
   row[ 4] ImageName
   row[ 5] ImageTitle
   */
   /* Get the type of answer */
   Gbl.Test.AnswerType = Tst_ConvertFromStrAnsTypDBToAnsTyp (row[0]);

   /* Get shuffle (row[1]) */
   Gbl.Test.Shuffle = (Str_ConvertToUpperLetter (row[1][0]) == 'Y');

   /* Get the stem of the question from the database (row[2]) */
   strncpy (Stem,row[2],Cns_MAX_BYTES_TEXT);
   Stem[Cns_MAX_BYTES_TEXT] = '\0';

   /* Get the feedback of the question from the database (row[3]) */
   Feedback[0] = '\0';
   if (row[3])
      if (row[3][0])
	{
	 strncpy (Feedback,row[3],Cns_MAX_BYTES_TEXT);
	 Feedback[Cns_MAX_BYTES_TEXT] = '\0';
	}

   /* Get the image name of the question from the database (row[4]) */
   Img_GetImageNameAndTitleFromRow (row[4],row[5],&Gbl.Test.Image);

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);

   /***** Get the tags from the database *****/
   NumRows = Tst_GetTagsQst (Gbl.Test.QstCod,&mysql_res);
   for (NumRow = 0;
	NumRow < NumRows;
	NumRow++)
     {
      row = mysql_fetch_row (mysql_res);
      strncpy (Gbl.Test.Tags.Txt[NumRow],row[0],Tst_MAX_BYTES_TAG);
      Gbl.Test.Tags.Txt[NumRow][Tst_MAX_BYTES_TAG] = '\0';
     }

   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);

   /***** Get the answers from the database *****/
   Gbl.Test.Answer.NumOptions = Tst_GetAnswersQst (Gbl.Test.QstCod,&mysql_res,false);	// Result: AnsInd,Answer,Correct,Feedback
   /*
   row[ 0] AnsInd
   row[ 1] Answer
   row[ 2] Feedback
   row[ 3] ImageName
   row[ 4] ImageTitle
   row[ 5] Correct
   */
   for (NumOpt = 0;
	NumOpt < Gbl.Test.Answer.NumOptions;
	NumOpt++)
     {
      row = mysql_fetch_row (mysql_res);
      switch (Gbl.Test.AnswerType)
	{
	 case Tst_ANS_INT:
	    if (Gbl.Test.Answer.NumOptions != 1)
	       Lay_ShowErrorAndExit ("Wrong answer.");
	    Gbl.Test.Answer.Integer = Tst_GetIntAnsFromStr (row[1]);
	    break;
	 case Tst_ANS_FLOAT:
	    if (Gbl.Test.Answer.NumOptions != 2)
	       Lay_ShowErrorAndExit ("Wrong answer.");
	    Gbl.Test.Answer.FloatingPoint[NumOpt] = Tst_GetFloatAnsFromStr (row[1]);
	    break;
	 case Tst_ANS_TRUE_FALSE:
	    if (Gbl.Test.Answer.NumOptions != 1)
	       Lay_ShowErrorAndExit ("Wrong answer.");
	    Gbl.Test.Answer.TF = row[1][0];
	    break;
	 case Tst_ANS_UNIQUE_CHOICE:
	 case Tst_ANS_MULTIPLE_CHOICE:
	 case Tst_ANS_TEXT:
	    if (Gbl.Test.Answer.NumOptions > Tst_MAX_OPTIONS_PER_QUESTION)
	       Lay_ShowErrorAndExit ("Wrong answer.");
	    if (!Tst_AllocateTextChoiceAnswer (NumOpt))
	       Lay_ShowErrorAndExit (Gbl.Message);

	    strncpy (Gbl.Test.Answer.Options[NumOpt].Text,row[1],Tst_MAX_BYTES_ANSWER_OR_FEEDBACK);
	    Gbl.Test.Answer.Options[NumOpt].Text[Tst_MAX_BYTES_ANSWER_OR_FEEDBACK] = '\0';

	    // Feedback (row[2]) is initialized to empty string
	    if (row[2])
	       if (row[2][0])
		 {
		  strncpy (Gbl.Test.Answer.Options[NumOpt].Feedback,row[2],Tst_MAX_BYTES_ANSWER_OR_FEEDBACK);
		  Gbl.Test.Answer.Options[NumOpt].Feedback[Tst_MAX_BYTES_ANSWER_OR_FEEDBACK] = '\0';
		 }

	    /* Copy image */
            Img_GetImageNameAndTitleFromRow (row[3],row[4],&Gbl.Test.Answer.Options[NumOpt].Image);

	    Gbl.Test.Answer.Options[NumOpt].Correct = (Str_ConvertToUpperLetter (row[5][0]) == 'Y');
	    break;
	 default:
	    break;
	}
     }
   /* Free structure that stores the query result */
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/***** Get possible image associated with a test question from database ******/
/*****************************************************************************/
//      NumOpt >= Tst_MAX_OPTIONS_PER_QUESTION ==> image associated to stem
// 0 <= NumOpt <  Tst_MAX_OPTIONS_PER_QUESTION ==> image associated to answer

static void Tst_GetImageFromDB (unsigned NumOpt,struct Image *Image)
  {
   char Query[256];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Build query depending on NumOpt *****/
   if (NumOpt < Tst_MAX_OPTIONS_PER_QUESTION)
      // Get image associated to answer
      sprintf (Query,"SELECT ImageName,ImageTitle FROM tst_answers"
		     " WHERE QstCod='%ld' AND AnsInd='%u'",
	       Gbl.Test.QstCod,NumOpt);
   else
      // Get image associated to stem
      sprintf (Query,"SELECT ImageName,ImageTitle FROM tst_questions"
		     " WHERE QstCod='%ld' AND CrsCod='%ld'",
	       Gbl.Test.QstCod,Gbl.CurrentCrs.Crs.CrsCod);

   /***** Query database *****/
   DB_QuerySELECT (Query,&mysql_res,"can not get image name");
   row = mysql_fetch_row (mysql_res);

   /***** Get the image name (row[0]) *****/
   Img_GetImageNameAndTitleFromRow (row[0],row[1],Image);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/** Convert a string with the type of answer in database to type of answer ***/
/*****************************************************************************/

Tst_AnswerType_t Tst_ConvertFromStrAnsTypDBToAnsTyp (const char *StrAnsTypeBD)
  {
   Tst_AnswerType_t AnsType;

   if (StrAnsTypeBD != NULL)
      for (AnsType = (Tst_AnswerType_t) 0;
	   AnsType < Tst_NUM_ANS_TYPES;
	   AnsType++)
         if (!strcmp (StrAnsTypeBD,Tst_StrAnswerTypesDB[AnsType]))
            return AnsType;

   Lay_ShowErrorAndExit ("Wrong type of answer.");
   return (Tst_AnswerType_t) 0;	// Not reached
  }

/*****************************************************************************/
/************ Convert a string with an unsigned to answer type ***************/
/*****************************************************************************/

static Tst_AnswerType_t Tst_ConvertFromUnsignedStrToAnsTyp (const char *UnsignedStr)
  {
   unsigned AnsType;

   if (sscanf (UnsignedStr,"%u",&AnsType) != 1)
      Lay_ShowErrorAndExit ("Wrong type of answer.");
   if (AnsType >= Tst_NUM_ANS_TYPES)
      Lay_ShowErrorAndExit ("Wrong type of answer.");
   return (Tst_AnswerType_t) AnsType;
  }

/*****************************************************************************/
/*************** Receive a question of the self-assessment test **************/
/*****************************************************************************/

void Tst_ReceiveQst (void)
  {
   char Stem[Cns_MAX_BYTES_TEXT+1];
   char Feedback[Cns_MAX_BYTES_TEXT+1];

   /***** Create test question *****/
   Tst_QstConstructor ();

   /***** Get parameters of the question from form *****/
   Stem[0] = Feedback[0] = '\0';
   Tst_GetQstFromForm (Stem,Feedback);

   /***** Make sure that tags, text and answer are not empty *****/
   if (Tst_CheckIfQstFormatIsCorrectAndCountNumOptions ())
     {
      /***** Move images to definitive directories *****/
      Tst_MoveImagesToDefinitiveDirectories ();

      /***** Insert or update question, tags and answer in the database *****/
      Tst_InsertOrUpdateQstTagsAnsIntoDB ();

      /***** Show the question just inserted in the database *****/
      Tst_ListOneQstToEdit ();
     }
   else	// Question is wrong
     {
      /***** Whether images has been received or not, reset images *****/
      Tst_InitImagesOfQuestion ();

      /***** Put form to edit question again *****/
      Tst_PutFormEditOneQst (Stem,Feedback);
     }

   /***** Destroy test question *****/
   Tst_QstDestructor ();
  }

/*****************************************************************************/
/**************** Get parameters of a test question from form ****************/
/*****************************************************************************/

static void Tst_GetQstFromForm (char *Stem,char *Feedback)
  {
   char UnsignedStr[10+1];
   char YN[1+1];
   unsigned NumTag;
   unsigned NumTagRead;
   unsigned NumOpt;
   char TagStr[6+10+1];
   char AnsStr[6+10+1];
   char FbStr[5+10+1];
   char StrMultiAns[Tst_MAX_SIZE_ANSWERS_ONE_QST+1];
   const char *Ptr;
   unsigned NumCorrectAns;
   char ParamAction[32];
   char ParamFile[32];
   char ParamTitle[32];

   /***** Get question code *****/
   Gbl.Test.QstCod = Tst_GetQstCod ();

   /***** Get answer type *****/
   Par_GetParToText ("AnswerType",UnsignedStr,10);
   Gbl.Test.AnswerType = Tst_ConvertFromUnsignedStrToAnsTyp (UnsignedStr);

   /***** Get question tags *****/
   for (NumTag = 0;
	NumTag < Tst_MAX_TAGS_PER_QUESTION;
	NumTag++)
     {
      sprintf (TagStr,"TagTxt%u",NumTag);
      Par_GetParToText (TagStr,Gbl.Test.Tags.Txt[NumTag],Tst_MAX_BYTES_TAG);

      if (Gbl.Test.Tags.Txt[NumTag][0])
        {
         Str_ChangeFormat (Str_FROM_FORM,Str_TO_TEXT,
                           Gbl.Test.Tags.Txt[NumTag],Tst_MAX_BYTES_TAG,true);
         /* Check if not repeated */
         for (NumTagRead = 0;
              NumTagRead < NumTag;
              NumTagRead++)
            if (!strcmp (Gbl.Test.Tags.Txt[NumTagRead],Gbl.Test.Tags.Txt[NumTag]))
              {
               Gbl.Test.Tags.Txt[NumTag][0] = '\0';
               break;
              }
        }
     }

   /***** Get question stem *****/
   Par_GetParToHTML ("Stem",Stem,Cns_MAX_BYTES_TEXT);

   /***** Get question feedback *****/
   Par_GetParToHTML ("Feedback",Feedback,Cns_MAX_BYTES_TEXT);

   /***** Get image associated to the stem *****/
   Img_GetImageFromForm (Tst_MAX_OPTIONS_PER_QUESTION,&Gbl.Test.Image,
                         Tst_GetImageFromDB,
                         "ImgAct","FilImg","TitImg",
	                 Tst_PHOTO_SAVED_MAX_WIDTH,
	                 Tst_PHOTO_SAVED_MAX_HEIGHT,
	                 Tst_PHOTO_SAVED_QUALITY);

   /***** Get answers *****/
   Gbl.Test.Shuffle = false;
   switch (Gbl.Test.AnswerType)
     {
      case Tst_ANS_INT:
         if (!Tst_AllocateTextChoiceAnswer (0))
            Lay_ShowErrorAndExit (Gbl.Message);

	 Par_GetParToText ("AnsInt",Gbl.Test.Answer.Options[0].Text,1+10);
	 break;
      case Tst_ANS_FLOAT:
         if (!Tst_AllocateTextChoiceAnswer (0))
            Lay_ShowErrorAndExit (Gbl.Message);
	 Par_GetParToText ("AnsFloatMin",Gbl.Test.Answer.Options[0].Text,Tst_MAX_BYTES_FLOAT_ANSWER);

         if (!Tst_AllocateTextChoiceAnswer (1))
            Lay_ShowErrorAndExit (Gbl.Message);
	 Par_GetParToText ("AnsFloatMax",Gbl.Test.Answer.Options[1].Text,Tst_MAX_BYTES_FLOAT_ANSWER);
	 break;
      case Tst_ANS_TRUE_FALSE:
	 Par_GetParToText ("AnsTF",YN,1);
	 Gbl.Test.Answer.TF = YN[0];
	 break;
      case Tst_ANS_UNIQUE_CHOICE:
      case Tst_ANS_MULTIPLE_CHOICE:
         /* Get shuffle */
         Par_GetParToText ("Shuffle",YN,1);
         Gbl.Test.Shuffle = (Str_ConvertToUpperLetter (YN[0]) == 'Y');
         // No break
      case Tst_ANS_TEXT:
         /* Get the texts of the answers */
         for (NumOpt = 0;
              NumOpt < Tst_MAX_OPTIONS_PER_QUESTION;
              NumOpt++)
           {
            if (!Tst_AllocateTextChoiceAnswer (NumOpt))
               Lay_ShowErrorAndExit (Gbl.Message);

            /* Get answer */
            sprintf (AnsStr,"AnsStr%u",NumOpt);
	    Par_GetParToHTML (AnsStr,Gbl.Test.Answer.Options[NumOpt].Text,Tst_MAX_BYTES_ANSWER_OR_FEEDBACK);
            Str_ReplaceSeveralSpacesForOne (Gbl.Test.Answer.Options[NumOpt].Text);	// Join several spaces into one in answer

            /* Get feedback */
            sprintf (FbStr,"FbStr%u",NumOpt);
	    Par_GetParToHTML (FbStr,Gbl.Test.Answer.Options[NumOpt].Feedback,Tst_MAX_BYTES_ANSWER_OR_FEEDBACK);

	    /* Get image associated to the answer */
	    if (Gbl.Test.AnswerType == Tst_ANS_UNIQUE_CHOICE ||
		Gbl.Test.AnswerType == Tst_ANS_MULTIPLE_CHOICE)
	      {
	       sprintf (ParamAction,"ImgAct%u",NumOpt);
	       sprintf (ParamFile  ,"FilImg%u",NumOpt);
	       sprintf (ParamTitle ,"TitImg%u",NumOpt);
	       Img_GetImageFromForm (NumOpt,&Gbl.Test.Answer.Options[NumOpt].Image,
				     Tst_GetImageFromDB,
				     ParamAction,ParamFile,ParamTitle,
				     Tst_PHOTO_SAVED_MAX_WIDTH,
				     Tst_PHOTO_SAVED_MAX_HEIGHT,
				     Tst_PHOTO_SAVED_QUALITY);
	      }
           }

         /* Get the numbers of correct answers */
         if (Gbl.Test.AnswerType == Tst_ANS_UNIQUE_CHOICE)
           {
	    Par_GetParToText ("AnsUni",UnsignedStr,10);
            if (UnsignedStr[0])
              {
               if (sscanf (UnsignedStr,"%u",&NumCorrectAns) != 1)
	          Lay_ShowErrorAndExit ("Wrong selected answer.");
               if (NumCorrectAns >= Tst_MAX_OPTIONS_PER_QUESTION)
	          Lay_ShowErrorAndExit ("Wrong selected answer.");
               Gbl.Test.Answer.Options[NumCorrectAns].Correct = true;
              }
           }
      	 else if (Gbl.Test.AnswerType == Tst_ANS_MULTIPLE_CHOICE)
           {
	    Par_GetParMultiToText ("AnsMulti",StrMultiAns,Tst_MAX_SIZE_ANSWERS_ONE_QST);
 	    Ptr = StrMultiAns;
            while (*Ptr)
              {
  	       Par_GetNextStrUntilSeparParamMult (&Ptr,UnsignedStr,10);
	       if (sscanf (UnsignedStr,"%u",&NumCorrectAns) != 1)
	          Lay_ShowErrorAndExit ("Wrong selected answer.");
               if (NumCorrectAns >= Tst_MAX_OPTIONS_PER_QUESTION)
	          Lay_ShowErrorAndExit ("Wrong selected answer.");
               Gbl.Test.Answer.Options[NumCorrectAns].Correct = true;
              }
           }
         else // Tst_ANS_TEXT
            for (NumOpt = 0;
        	 NumOpt < Tst_MAX_OPTIONS_PER_QUESTION;
        	 NumOpt++)
               if (Gbl.Test.Answer.Options[NumOpt].Text[0])
                  Gbl.Test.Answer.Options[NumOpt].Correct = true;	// All the answers are correct
	 break;
      default:
         break;
     }

   /***** Adjust global variables related to this test question *****/
   for (NumTag = 0, Gbl.Test.Tags.Num = 0;
        NumTag < Tst_MAX_TAGS_PER_QUESTION;
        NumTag++)
      if (Gbl.Test.Tags.Txt[NumTag][0])
         Gbl.Test.Tags.Num++;
   Gbl.Test.Stem.Text = Stem;
   Gbl.Test.Stem.Length = strlen (Gbl.Test.Stem.Text);
   Gbl.Test.Feedback.Text = Feedback;
   Gbl.Test.Feedback.Length = strlen (Gbl.Test.Feedback.Text);
  }

/*****************************************************************************/
/*********************** Check if a question is correct **********************/
/*****************************************************************************/
// Returns false if question format is wrong
// Counts Gbl.Test.Answer.NumOptions
// Computes Gbl.Test.Answer.Integer and Gbl.Test.Answer.FloatingPoint[0..1]

bool Tst_CheckIfQstFormatIsCorrectAndCountNumOptions (void)
  {
   extern const char *Txt_Error_receiving_or_processing_image;
   extern const char *Txt_You_must_type_at_least_one_tag_for_the_question;
   extern const char *Txt_You_must_type_the_stem_of_the_question;
   extern const char *Txt_You_must_select_a_T_F_answer;
   extern const char *Txt_You_can_not_leave_empty_intermediate_answers;
   extern const char *Txt_You_must_type_at_least_the_first_two_answers;
   extern const char *Txt_You_must_mark_an_answer_as_correct;
   extern const char *Txt_You_must_type_at_least_the_first_answer;
   extern const char *Txt_You_must_enter_an_integer_value_as_the_correct_answer;
   extern const char *Txt_You_must_enter_the_range_of_floating_point_values_allowed_as_answer;
   extern const char *Txt_The_lower_limit_of_correct_answers_must_be_less_than_or_equal_to_the_upper_limit;
   unsigned NumOpt;
   unsigned NumLastOpt;
   bool ThereIsEndOfAnswers;
   unsigned i;

   if ((Gbl.Test.Image.Action == Img_ACTION_NEW_IMAGE ||	// Upload new image
        Gbl.Test.Image.Action == Img_ACTION_CHANGE_IMAGE) &&	// Replace existing image by new image
       Gbl.Test.Image.Status != Img_FILE_PROCESSED)
     {
      Lay_ShowAlert (Lay_WARNING,Txt_Error_receiving_or_processing_image);
      return false;
     }

   /***** This function also counts the number of options. Initialize this number to 0. *****/
   Gbl.Test.Answer.NumOptions = 0;

   /***** A question must have at least one tag *****/
   if (!Gbl.Test.Tags.Num) // There are no tags with text
     {
      Lay_ShowAlert (Lay_WARNING,Txt_You_must_type_at_least_one_tag_for_the_question);
      return false;
     }

   /***** A question must have a stem*****/
   if (!Gbl.Test.Stem.Length)
     {
      Lay_ShowAlert (Lay_WARNING,Txt_You_must_type_the_stem_of_the_question);
      return false;
     }

   /***** Check answer *****/
   switch (Gbl.Test.AnswerType)
     {
      case Tst_ANS_INT:
         if (!Gbl.Test.Answer.Options[0].Text)
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_enter_an_integer_value_as_the_correct_answer);
            return false;
           }
         if (!Gbl.Test.Answer.Options[0].Text[0])
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_enter_an_integer_value_as_the_correct_answer);
            return false;
           }
         Gbl.Test.Answer.Integer = Tst_GetIntAnsFromStr (Gbl.Test.Answer.Options[0].Text);
         Gbl.Test.Answer.NumOptions = 1;
         break;
      case Tst_ANS_FLOAT:
         if (!Gbl.Test.Answer.Options[0].Text ||
             !Gbl.Test.Answer.Options[1].Text)
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_enter_the_range_of_floating_point_values_allowed_as_answer);
            return false;
           }
         if (!Gbl.Test.Answer.Options[0].Text[0] ||
             !Gbl.Test.Answer.Options[1].Text[0])
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_enter_the_range_of_floating_point_values_allowed_as_answer);
            return false;
           }
         for (i = 0;
              i < 2;
              i++)
            Gbl.Test.Answer.FloatingPoint[i] = Tst_GetFloatAnsFromStr (Gbl.Test.Answer.Options[i].Text);
         if (Gbl.Test.Answer.FloatingPoint[0] >
             Gbl.Test.Answer.FloatingPoint[1])
           {
            Lay_ShowAlert (Lay_WARNING,Txt_The_lower_limit_of_correct_answers_must_be_less_than_or_equal_to_the_upper_limit);
            return false;
           }
         Gbl.Test.Answer.NumOptions = 2;
         break;
      case Tst_ANS_TRUE_FALSE:
         if (Gbl.Test.Answer.TF != 'T' &&
             Gbl.Test.Answer.TF != 'F')
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_select_a_T_F_answer);
            return false;
           }
         Gbl.Test.Answer.NumOptions = 1;
         break;
      case Tst_ANS_UNIQUE_CHOICE:
      case Tst_ANS_MULTIPLE_CHOICE:
         if (!Gbl.Test.Answer.Options[0].Text)		// If the first answer is empty
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_type_at_least_the_first_two_answers);
            return false;
           }
         if (!Gbl.Test.Answer.Options[0].Text[0])	// If the first answer is empty
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_type_at_least_the_first_two_answers);
            return false;
           }

         for (NumOpt = 0, NumLastOpt = 0, ThereIsEndOfAnswers = false;
              NumOpt < Tst_MAX_OPTIONS_PER_QUESTION;
              NumOpt++)
            if (Gbl.Test.Answer.Options[NumOpt].Text)
              {
               if (Gbl.Test.Answer.Options[NumOpt].Text[0])
                 {
                  if (ThereIsEndOfAnswers)
                    {
                     Lay_ShowAlert (Lay_WARNING,Txt_You_can_not_leave_empty_intermediate_answers);
                     return false;
                    }
                  NumLastOpt = NumOpt;
                  Gbl.Test.Answer.NumOptions++;
                 }
               else
                  ThereIsEndOfAnswers = true;
              }
            else
               ThereIsEndOfAnswers = true;

         if (NumLastOpt < 1)
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_type_at_least_the_first_two_answers);
            return false;
           }

         for (NumOpt = 0;
              NumOpt <= NumLastOpt;
              NumOpt++)
            if (Gbl.Test.Answer.Options[NumOpt].Correct)
               break;
         if (NumOpt > NumLastOpt)
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_mark_an_answer_as_correct);
            return false;
           }
         break;
      case Tst_ANS_TEXT:
         if (!Gbl.Test.Answer.Options[0].Text)		// If the first answer is empty
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_type_at_least_the_first_two_answers);
            return false;
           }
         if (!Gbl.Test.Answer.Options[0].Text[0])  // If the first answer is empty
           {
            Lay_ShowAlert (Lay_WARNING,Txt_You_must_type_at_least_the_first_answer);
            return false;
           }

         for (NumOpt=0, ThereIsEndOfAnswers=false;
              NumOpt<Tst_MAX_OPTIONS_PER_QUESTION;
              NumOpt++)
            if (Gbl.Test.Answer.Options[NumOpt].Text)
              {
               if (Gbl.Test.Answer.Options[NumOpt].Text[0])
                 {
                  if (ThereIsEndOfAnswers)
                    {
                     Lay_ShowAlert (Lay_WARNING,Txt_You_can_not_leave_empty_intermediate_answers);
                     return false;
                    }
                  Gbl.Test.Answer.NumOptions++;
                 }
               else
                  ThereIsEndOfAnswers = true;
              }
            else
               ThereIsEndOfAnswers = true;
         break;
      default:
         break;
     }

    return true;	// Question format without errors
   }

/*****************************************************************************/
/* Move images associates to a test question to their definitive directories */
/*****************************************************************************/

static void Tst_MoveImagesToDefinitiveDirectories (void)
  {
   unsigned NumOpt;

   /****** Move image associated to question stem *****/
   if (Gbl.Test.QstCod > 0 &&				// Question already exists
       Gbl.Test.Image.Action != Img_ACTION_KEEP_IMAGE)	// Don't keep the current image
      /* Remove possible file with the old image
	 (the new image file is already processed
	  and moved to the definitive directory) */
      Tst_RemoveImgFileFromStemOfQst (Gbl.CurrentCrs.Crs.CrsCod,Gbl.Test.QstCod);

   if ((Gbl.Test.Image.Action == Img_ACTION_NEW_IMAGE ||	// Upload new image
	Gbl.Test.Image.Action == Img_ACTION_CHANGE_IMAGE) &&	// Replace existing image by new image
        Gbl.Test.Image.Status == Img_FILE_PROCESSED)		// The new image received has been processed
      /* Move processed image to definitive directory */
      Img_MoveImageToDefinitiveDirectory (&Gbl.Test.Image);

   /****** Move images associated to answers *****/
   if (Gbl.Test.AnswerType == Tst_ANS_UNIQUE_CHOICE ||
       Gbl.Test.AnswerType == Tst_ANS_MULTIPLE_CHOICE)
      for (NumOpt = 0;
	   NumOpt < Gbl.Test.Answer.NumOptions;
	   NumOpt++)
	{
	 if (Gbl.Test.QstCod > 0 &&							// Question already exists
             Gbl.Test.Answer.Options[NumOpt].Image.Action != Img_ACTION_KEEP_IMAGE)	// Don't keep the current image
	    /* Remove possible file with the old image
	       (the new image file is already processed
		and moved to the definitive directory) */
	    Tst_RemoveImgFileFromAnsOfQst (Gbl.CurrentCrs.Crs.CrsCod,Gbl.Test.QstCod,NumOpt);

	 if ((Gbl.Test.Answer.Options[NumOpt].Image.Action == Img_ACTION_NEW_IMAGE ||		// Upload new image
	      Gbl.Test.Answer.Options[NumOpt].Image.Action == Img_ACTION_CHANGE_IMAGE) &&	// Replace existing image by new image
	      Gbl.Test.Answer.Options[NumOpt].Image.Status == Img_FILE_PROCESSED)		// The new image received has been processed
	    /* Move processed image to definitive directory */
	    Img_MoveImageToDefinitiveDirectory (&Gbl.Test.Answer.Options[NumOpt].Image);
	}
  }

/*****************************************************************************/
/******************** Get a integer number from a string *********************/
/*****************************************************************************/

long Tst_GetIntAnsFromStr (char *Str)
  {
   long LongNum;

   if (Str == NULL)
      return 0.0;

   /***** The string is "scanned" as long *****/
   if (sscanf (Str,"%ld",&LongNum) != 1)	// If the string does not hold a valid integer number...
     {
      LongNum = 0L;	// ...the number is reset to 0
      Str[0] = '\0';	// ...and the string is reset to ""
     }

   return LongNum;
  }

/*****************************************************************************/
/************ Get a float number from a string in floating point *************/
/*****************************************************************************/

double Tst_GetFloatAnsFromStr (char *Str)
  {
   double DoubleNum;

   if (Str == NULL)
      return 0.0;

   /***** Change commnas to points *****/
   Str_ConvertStrFloatCommaToStrFloatPoint (Str);

   /***** The string is "scanned" in floating point (it must have a point, not a colon as decimal separator) *****/
   setlocale (LC_NUMERIC,"en_US.utf8");	// To get decimal point
   if (sscanf (Str,"%lg",&DoubleNum) != 1)	// If the string does not hold a valid floating point number...
     {
      DoubleNum = 0.0;	// ...the number is reset to 0
      Str[0] = '\0';	// ...and the string is reset to ""
     }
   setlocale (LC_NUMERIC,"es_ES.utf8");	// Return to spanish system (TODO: this should be internationalized!!!!!!!)

   return DoubleNum;
  }

/*****************************************************************************/
/***************** Check if this tag exists for current course ***************/
/*****************************************************************************/

static long Tst_GetTagCodFromTagTxt (const char *TagTxt)
  {
   char Query[256+Tst_MAX_BYTES_TAG];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned long NumRows;
   long TagCod = -1L;	// -1 means that the tag does not exist in database
   bool Error = false;

   /***** Get tag code from database *****/
   sprintf (Query,"SELECT TagCod FROM tst_tags"
                  " WHERE tst_tags.CrsCod='%ld' AND tst_tags.TagTxt='%s'",
            Gbl.CurrentCrs.Crs.CrsCod,TagTxt);
   NumRows = DB_QuerySELECT (Query,&mysql_res,"can not get tag");

   if (NumRows == 1)
     {
      /***** Get tag code *****/
      row = mysql_fetch_row (mysql_res);
      if ((TagCod = Str_ConvertStrCodToLongCod (row[0])) < 0)
        {
         strcpy (Gbl.Message,"Wrong code of tag.");
         Error = true;
        }
     }
   else if (NumRows > 1)
     {
      strcpy (Gbl.Message,"Duplicated tag.");
      Error = true;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   if (Error)
      Lay_ShowErrorAndExit (Gbl.Message);

   return TagCod;
  }

/*****************************************************************************/
/********************* Insert new tag into tst_tags table ********************/
/*****************************************************************************/

static long Tst_CreateNewTag (long CrsCod,const char *TagTxt)
  {
   char Query[256+Tst_MAX_BYTES_TAG];

   /***** Insert new tag into tst_tags table *****/
   sprintf (Query,"INSERT INTO tst_tags (CrsCod,ChangeTime,TagTxt,TagHidden)"
                  " VALUES ('%ld',NOW(),'%s','N')",
            CrsCod,TagTxt);
   return DB_QueryINSERTandReturnCode (Query,"can not create new tag");
  }

/*****************************************************************************/
/********** Change visibility of an existing tag into tst_tags table *********/
/*****************************************************************************/

static void Tst_EnableOrDisableTag (long TagCod,bool TagHidden)
  {
   char Query[512];

   /***** Insert new tag into tst_tags table *****/
   sprintf (Query,"UPDATE tst_tags SET TagHidden='%c',ChangeTime=NOW()"
                  " WHERE TagCod='%ld' AND CrsCod='%ld'",
            TagHidden ? 'Y' :
        	        'N',
            TagCod,Gbl.CurrentCrs.Crs.CrsCod);
   DB_QueryUPDATE (Query,"can not update the visibility of a tag");
  }

/*****************************************************************************/
/********************* Put icon to remove one question ***********************/
/*****************************************************************************/

static void Tst_PutIconToRemoveOneQst (void)
  {
   extern const char *Txt_Remove;

   Lay_PutContextualLink (ActReqRemTstQst,Tst_PutParamsRemoveOneQst,
                          "remove-on64x64.png",Txt_Remove,NULL);
  }

/*****************************************************************************/
/****************** Put parameter to remove one question *********************/
/*****************************************************************************/

static void Tst_PutParamsRemoveOneQst (void)
  {
   Par_PutHiddenParamLong ("QstCod",Gbl.Test.QstCod);
   Par_PutHiddenParamChar ("OnlyThisQst",'Y');
  }

/*****************************************************************************/
/******************** Request the removal of a question **********************/
/*****************************************************************************/

void Tst_RequestRemoveQst (void)
  {
   extern const char *Txt_Do_you_really_want_to_remove_the_question_X;
   extern const char *Txt_Remove_question;
   char YN[1+1];
   bool EditingOnlyThisQst;

   /***** Get main parameters from form *****/
   /* Get the question code */
   Gbl.Test.QstCod = Tst_GetQstCod ();
   if (Gbl.Test.QstCod <= 0)
      Lay_ShowErrorAndExit ("Wrong code of question.");

   /* Get a parameter that indicates whether it's necessary
      to continue listing the rest of questions */
   Par_GetParToText ("OnlyThisQst",YN,1);
   EditingOnlyThisQst = (Str_ConvertToUpperLetter (YN[0]) == 'Y');

   /***** Start form *****/
   Act_FormStart (ActRemTstQst);
   Par_PutHiddenParamLong ("QstCod",Gbl.Test.QstCod);
   if (EditingOnlyThisQst)
      Par_PutHiddenParamChar ("OnlyThisQst",'Y');
   else	// Editing a list of questions
     {
      /* Get and write other parameters related to the listing of questions */
      if (Tst_GetParamsTst ())
	{
	 Sta_WriteParamsDatesSeeAccesses ();
	 Tst_WriteParamEditQst ();
	}
      Tst_FreeTagsList ();
     }

   /***** Ask for confirmation of removing *****/
   sprintf (Gbl.Message,Txt_Do_you_really_want_to_remove_the_question_X,
	    (unsigned long) Gbl.Test.QstCod);
   Lay_ShowAlert (Lay_WARNING,Gbl.Message);
   Lay_PutRemoveButton (Txt_Remove_question);

   /***** End form *****/
   Act_FormEnd ();

   /***** Continue editing questions *****/
   if (EditingOnlyThisQst)
      Tst_ListOneQstToEdit ();
   else
      Tst_ListQuestionsToEdit ();
  }

/*****************************************************************************/
/***************************** Remove a question *****************************/
/*****************************************************************************/

void Tst_RemoveQst (void)
  {
   extern const char *Txt_Question_removed;
   char Query[512];
   char YN[1+1];
   bool EditingOnlyThisQst;

   /***** Get the question code *****/
   Gbl.Test.QstCod = Tst_GetQstCod ();
   if (Gbl.Test.QstCod <= 0)
      Lay_ShowErrorAndExit ("Wrong code of question.");

   /***** Get a parameter that indicates whether it's necessary
          to continue listing the rest of questions ******/
   Par_GetParToText ("OnlyThisQst",YN,1);
   EditingOnlyThisQst = (Str_ConvertToUpperLetter (YN[0]) == 'Y');

   /***** Remove images associated to question *****/
   Tst_RemoveAllImgFilesFromAnsOfQst (Gbl.CurrentCrs.Crs.CrsCod,Gbl.Test.QstCod);
   Tst_RemoveImgFileFromStemOfQst (Gbl.CurrentCrs.Crs.CrsCod,Gbl.Test.QstCod);

   /***** Remove the question from all the tables *****/
   /* Remove answers and tags from this test question */
   Tst_RemAnsFromQst ();
   Tst_RemTagsFromQst ();
   Tst_RemoveUnusedTagsFromCurrentCrs ();

   /* Remove the question itself */
   sprintf (Query,"DELETE FROM tst_questions"
                  " WHERE QstCod='%ld' AND CrsCod='%ld'",
            Gbl.Test.QstCod,Gbl.CurrentCrs.Crs.CrsCod);
   DB_QueryDELETE (Query,"can not remove a question");

   if (!mysql_affected_rows (&Gbl.mysql))
      Lay_ShowErrorAndExit ("The question to be removed does not exist or belongs to another course.");

   /***** Write message *****/
   Lay_ShowAlert (Lay_SUCCESS,Txt_Question_removed);

   /***** Continue editing questions *****/
   if (!EditingOnlyThisQst)
      Tst_ListQuestionsToEdit ();
  }

/*****************************************************************************/
/*********************** Change the shuffle of a question ********************/
/*****************************************************************************/

void Tst_ChangeShuffleQst (void)
  {
   extern const char *Txt_The_answers_of_the_question_with_code_X_will_appear_shuffled;
   extern const char *Txt_The_answers_of_the_question_with_code_X_will_appear_without_shuffling;
   char Query[512];
   char YN[1+1];
   bool EditingOnlyThisQst;
   bool Shuffle;

   /***** Get the question code *****/
   Gbl.Test.QstCod = Tst_GetQstCod ();
   if (Gbl.Test.QstCod <= 0)
      Lay_ShowErrorAndExit ("Wrong code of question.");

   /***** Get a parameter that indicates whether it's necessary to continue listing the rest of questions ******/
   Par_GetParToText ("OnlyThisQst",YN,1);
   EditingOnlyThisQst = (Str_ConvertToUpperLetter (YN[0]) == 'Y');

   /***** Get a parameter that indicates whether it's possible to shuffle the answers of this question ******/
   Par_GetParToText ("Shuffle",YN,1);
   Shuffle = (Str_ConvertToUpperLetter (YN[0]) == 'Y');

   /***** Remove the question from all the tables *****/
   /* Update the question changing the current shuffle */
   sprintf (Query,"UPDATE tst_questions SET Shuffle='%c'"
                  " WHERE QstCod='%ld' AND CrsCod='%ld'",
            Shuffle ? 'Y' :
        	      'N',
            Gbl.Test.QstCod,Gbl.CurrentCrs.Crs.CrsCod);
   DB_QueryUPDATE (Query,"can not update the shuffle type of a question");

   /***** Write message *****/
   sprintf (Gbl.Message,
            Shuffle ? Txt_The_answers_of_the_question_with_code_X_will_appear_shuffled :
                      Txt_The_answers_of_the_question_with_code_X_will_appear_without_shuffling,
            Gbl.Test.QstCod);
   Lay_ShowAlert (Lay_SUCCESS,Gbl.Message);

   /***** Continue editing questions *****/
   if (EditingOnlyThisQst)
      Tst_ListOneQstToEdit ();
   else
      Tst_ListQuestionsToEdit ();
  }

/*****************************************************************************/
/************ Get the parameter with the code of a test question *************/
/*****************************************************************************/

static long Tst_GetQstCod (void)
  {
   char LongStr[1+10+1];

   Par_GetParToText ("QstCod",LongStr,1+10);
   return Str_ConvertStrCodToLongCod (LongStr);
  }

/*****************************************************************************/
/******** Insert or update question, tags and anser in the database **********/
/*****************************************************************************/

void Tst_InsertOrUpdateQstTagsAnsIntoDB (void)
  {
   /***** Insert or update question in the table of questions *****/
   Tst_InsertOrUpdateQstIntoDB ();

   /***** Insert tags in the tags table *****/
   Tst_InsertTagsIntoDB ();

   /***** Remove unused tags in current course *****/
   Tst_RemoveUnusedTagsFromCurrentCrs ();

   /***** Insert answers in the answers table *****/
   Tst_InsertAnswersIntoDB ();
  }

/*****************************************************************************/
/*********** Insert or update question in the table of questions *************/
/*****************************************************************************/

static void Tst_InsertOrUpdateQstIntoDB (void)
  {
   char *Query;

   /***** Allocate space for query *****/
   if ((Query = malloc (512 +
                        Gbl.Test.Stem.Length +
                        Gbl.Test.Feedback.Length +
                        Img_MAX_BYTES_TITLE)) == NULL)
      Lay_ShowErrorAndExit ("Not enough memory to store database query.");

   if (Gbl.Test.QstCod < 0)	// It's a new question
     {
      /***** Insert question in the table of questions *****/
      sprintf (Query,"INSERT INTO tst_questions"
	             " (CrsCod,EditTime,AnsType,Shuffle,"
	             "Stem,Feedback,ImageName,ImageTitle,"
	             "NumHits,Score)"
                     " VALUES ('%ld',NOW(),'%s','%c',"
                     "'%s','%s','%s','%s',"
                     "'0','0')",
               Gbl.CurrentCrs.Crs.CrsCod,
               Tst_StrAnswerTypesDB[Gbl.Test.AnswerType],
               Gbl.Test.Shuffle ? 'Y' :
        	                  'N',
               Gbl.Test.Stem.Text,
               Gbl.Test.Feedback.Text ? Gbl.Test.Feedback.Text : "",
               Gbl.Test.Image.Name,
               Gbl.Test.Image.Title ? Gbl.Test.Image.Title : "");
      Gbl.Test.QstCod = DB_QueryINSERTandReturnCode (Query,"can not create question");

      /* Update image status */
      if (Gbl.Test.Image.Name[0])
	 Gbl.Test.Image.Status = Img_NAME_STORED_IN_DB;
     }
   else				// It's an existing question
     {
      /***** Update existing question *****/
      /* Update question in database */
      sprintf (Query,"UPDATE tst_questions"
		     " SET EditTime=NOW(),AnsType='%s',Shuffle='%c',"
		     "Stem='%s',Feedback='%s',ImageName='%s',ImageTitle='%s'"
		     " WHERE QstCod='%ld' AND CrsCod='%ld'",
	       Tst_StrAnswerTypesDB[Gbl.Test.AnswerType],
	       Gbl.Test.Shuffle ? 'Y' :
				  'N',
	       Gbl.Test.Stem.Text,
	       Gbl.Test.Feedback.Text ? Gbl.Test.Feedback.Text : "",
	       Gbl.Test.Image.Name,
	       Gbl.Test.Image.Title ? Gbl.Test.Image.Title : "",
	       Gbl.Test.QstCod,Gbl.CurrentCrs.Crs.CrsCod);
      DB_QueryUPDATE (Query,"can not update question");

      /* Update image status */
      if (Gbl.Test.Image.Name[0])
	 Gbl.Test.Image.Status = Img_NAME_STORED_IN_DB;

      /* Remove answers and tags from this test question */
      Tst_RemAnsFromQst ();
      Tst_RemTagsFromQst ();
     }

   /***** Free space user for query *****/
   free ((void *) Query);
  }

/*****************************************************************************/
/*********************** Insert tags in the tags table ***********************/
/*****************************************************************************/

static void Tst_InsertTagsIntoDB (void)
  {
   char Query[256];
   unsigned NumTag;
   unsigned TagIdx;
   long TagCod;

   /***** For each tag... *****/
   for (NumTag = 0, TagIdx = 0;
        TagIdx < Gbl.Test.Tags.Num;
        NumTag++)
      if (Gbl.Test.Tags.Txt[NumTag][0])
        {
         /***** Check if this tag exists for current course *****/
         if ((TagCod = Tst_GetTagCodFromTagTxt (Gbl.Test.Tags.Txt[NumTag])) < 0)
            /* This tag is new for current course. Add it to tags table */
            TagCod = Tst_CreateNewTag (Gbl.CurrentCrs.Crs.CrsCod,Gbl.Test.Tags.Txt[NumTag]);

         /***** Insert tag in tst_question_tags *****/
         sprintf (Query,"INSERT INTO tst_question_tags (QstCod,TagCod,TagInd)"
                        " VALUES ('%ld','%ld','%u')",
                  Gbl.Test.QstCod,TagCod,TagIdx);
         DB_QueryINSERT (Query,"can not create tag");

         TagIdx++;
        }
  }

/*****************************************************************************/
/******************* Insert answers in the answers table *********************/
/*****************************************************************************/

static void Tst_InsertAnswersIntoDB (void)
  {
   char *Query;
   unsigned NumOpt;
   unsigned i;

   /***** Allocate space for query *****/
   if ((Query = malloc (256+Tst_MAX_BYTES_ANSWER_OR_FEEDBACK*2)) == NULL)
      Lay_ShowErrorAndExit ("Not enough memory to store database query.");

   /***** Insert answers in the answers table *****/
   switch (Gbl.Test.AnswerType)
     {
      case Tst_ANS_INT:
         sprintf (Query,"INSERT INTO tst_answers"
                        " (QstCod,AnsInd,Answer,Feedback,ImageName,ImageTitle,Correct)"
                        " VALUES (%ld,0,'%ld','','','','Y')",
                  Gbl.Test.QstCod,
                  Gbl.Test.Answer.Integer);
         DB_QueryINSERT (Query,"can not create answer");
         break;
      case Tst_ANS_FLOAT:
         setlocale (LC_NUMERIC,"en_US.utf8");	// To print the floating point as a dot
   	 for (i = 0;
   	      i < 2;
   	      i++)
           {
            sprintf (Query,"INSERT INTO tst_answers"
                           " (QstCod,AnsInd,Answer,Feedback,ImageName,ImageTitle,Correct)"
                           " VALUES (%ld,%u,'%lg','','','','Y')",
                     Gbl.Test.QstCod,i,
                     Gbl.Test.Answer.FloatingPoint[i]);
            DB_QueryINSERT (Query,"can not create answer");
           }
         setlocale (LC_NUMERIC,"es_ES.utf8");	// Return to spanish system (TODO: this should be internationalized!!!!!!!)
         break;
      case Tst_ANS_TRUE_FALSE:
         sprintf (Query,"INSERT INTO tst_answers"
                        " (QstCod,AnsInd,Answer,Feedback,ImageName,ImageTitle,Correct)"
                        " VALUES (%ld,0,'%c','','','','Y')",
                  Gbl.Test.QstCod,
                  Gbl.Test.Answer.TF);
         DB_QueryINSERT (Query,"can not create answer");
         break;
      case Tst_ANS_UNIQUE_CHOICE:
      case Tst_ANS_MULTIPLE_CHOICE:
      case Tst_ANS_TEXT:
         for (NumOpt = 0;
              NumOpt < Gbl.Test.Answer.NumOptions;
              NumOpt++)
            if (Gbl.Test.Answer.Options[NumOpt].Text[0])
              {
               sprintf (Query,"INSERT INTO tst_answers"
                              " (QstCod,AnsInd,Answer,Feedback,ImageName,ImageTitle,Correct)"
                              " VALUES ('%ld','%u','%s','%s','%s','%s','%c')",
                        Gbl.Test.QstCod,NumOpt,
                        Gbl.Test.Answer.Options[NumOpt].Text,
                        Gbl.Test.Answer.Options[NumOpt].Feedback ? Gbl.Test.Answer.Options[NumOpt].Feedback : "",
                        Gbl.Test.Answer.Options[NumOpt].Image.Name,
                        Gbl.Test.Answer.Options[NumOpt].Image.Title ? Gbl.Test.Answer.Options[NumOpt].Image.Title : "",
                        Gbl.Test.Answer.Options[NumOpt].Correct ? 'Y' :
                                                                  'N');
               DB_QueryINSERT (Query,"can not create answer");

               /* Update image status */
	       if (Gbl.Test.Answer.Options[NumOpt].Image.Name[0])
		  Gbl.Test.Answer.Options[NumOpt].Image.Status = Img_NAME_STORED_IN_DB;
              }
	 break;
      default:
         break;
     }

   /***** Free space allocated for query *****/
   free ((void *) Query);
  }

/*****************************************************************************/
/******************** Remove answers from a test question ********************/
/*****************************************************************************/

static void Tst_RemAnsFromQst (void)
  {
   char Query[512];

   /***** Remove answers *****/
   sprintf (Query,"DELETE FROM tst_answers WHERE QstCod='%ld'",
            Gbl.Test.QstCod);
   DB_QueryDELETE (Query,"can not remove the answers of a question");
  }

/*****************************************************************************/
/************************** Remove tags from a test question *****************/
/*****************************************************************************/

static void Tst_RemTagsFromQst (void)
  {
   char Query[512];

   /***** Remove tags *****/
   sprintf (Query,"DELETE FROM tst_question_tags WHERE QstCod='%ld'",
            Gbl.Test.QstCod);
   DB_QueryDELETE (Query,"can not remove the tags of a question");
  }

/*****************************************************************************/
/******************** Remove unused tags in current course *******************/
/*****************************************************************************/

static void Tst_RemoveUnusedTagsFromCurrentCrs (void)
  {
   char Query[1024];

   /***** Remove unused tags from tst_tags *****/
   sprintf (Query,"DELETE FROM tst_tags WHERE CrsCod='%ld' AND TagCod NOT IN"
                  " (SELECT DISTINCT tst_question_tags.TagCod FROM tst_questions,tst_question_tags"
                  " WHERE tst_questions.CrsCod='%ld' AND tst_questions.QstCod=tst_question_tags.QstCod)",
            Gbl.CurrentCrs.Crs.CrsCod,Gbl.CurrentCrs.Crs.CrsCod);
   DB_QueryDELETE (Query,"can not remove unused tags");
  }

/*****************************************************************************/
/******** Remove one file associated to one stem of one test question ********/
/*****************************************************************************/

static void Tst_RemoveImgFileFromStemOfQst (long CrsCod,long QstCod)
  {
   char Query[256];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get names of images associated to stems of test questions from database *****/
   sprintf (Query,"SELECT ImageName FROM tst_questions"
		  " WHERE QstCod='%ld' AND CrsCod='%ld'",
	    QstCod,CrsCod);
   if (DB_QuerySELECT (Query,&mysql_res,"can not get test images"))
     {
      /***** Get image name (row[0]) *****/
      row = mysql_fetch_row (mysql_res);

      /***** Remove image file *****/
      Img_RemoveImageFile (row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/****** Remove all image files associated to stems of all test questions *****/
/****** in a course                                                      *****/
/*****************************************************************************/

static void Tst_RemoveAllImgFilesFromStemOfAllQstsInCrs (long CrsCod)
  {
   char Query[256];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumImages;
   unsigned NumImg;

   /***** Get names of images associated to stems of test questions from database *****/
   sprintf (Query,"SELECT ImageName FROM tst_questions"
		  " WHERE CrsCod='%ld'",
	    CrsCod);
   NumImages = (unsigned) DB_QuerySELECT (Query,&mysql_res,"can not get test images");

   /***** Go over result removing image files *****/
   for (NumImg = 0;
	NumImg < NumImages;
	NumImg++)
     {
      /***** Get image name (row[0]) *****/
      row = mysql_fetch_row (mysql_res);

      /***** Remove image file *****/
      Img_RemoveImageFile (row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/**** Remove one image file associated to one answer of one test question ****/
/*****************************************************************************/

static void Tst_RemoveImgFileFromAnsOfQst (long CrsCod,long QstCod,unsigned AnsInd)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get names of images associated to answers of test questions from database *****/
   sprintf (Query,"SELECT tst_answers.ImageName"
		  " FROM tst_questions,tst_answers"
		  " WHERE tst_questions.CrsCod='%ld'"	// Extra check
		  " AND tst_questions.QstCod='%ld'"	// Extra check
		  " AND tst_questions.QstCod=tst_answers.QstCod"
		  " AND tst_answers.QstCod='%ld'"
		  " AND tst_answers.AnsInd='%u'",
	    CrsCod,QstCod,QstCod,AnsInd);
   if (DB_QuerySELECT (Query,&mysql_res,"can not get test images"))
     {
      /***** Get image name (row[0]) *****/
      row = mysql_fetch_row (mysql_res);

      /***** Remove image file *****/
      Img_RemoveImageFile (row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*** Remove all image files associated to all answers of one test question ***/
/*****************************************************************************/

static void Tst_RemoveAllImgFilesFromAnsOfQst (long CrsCod,long QstCod)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumImages;
   unsigned NumImg;

   /***** Get names of images associated to answers of test questions from database *****/
   sprintf (Query,"SELECT tst_answers.ImageName"
		  " FROM tst_questions,tst_answers"
		  " WHERE tst_questions.CrsCod='%ld'"	// Extra check
		  " AND tst_questions.QstCod='%ld'"	// Extra check
		  " AND tst_questions.QstCod=tst_answers.QstCod"
		  " AND tst_answers.QstCod='%ld'",
	    CrsCod,QstCod,QstCod);
   NumImages = (unsigned) DB_QuerySELECT (Query,&mysql_res,"can not get test images");

   /***** Go over result removing image files *****/
   for (NumImg = 0;
	NumImg < NumImages;
	NumImg++)
     {
      /***** Get image name (row[0]) *****/
      row = mysql_fetch_row (mysql_res);

      /***** Remove image file *****/
      Img_RemoveImageFile (row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/** Remove all image files associated to all answers of all test questions ***/
/** in a course                                                            ***/
/*****************************************************************************/

static void Tst_RemoveAllImgFilesFromAnsOfAllQstsInCrs (long CrsCod)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumImages;
   unsigned NumImg;

   /***** Get names of images associated to answers of test questions from database *****/
   sprintf (Query,"SELECT tst_answers.ImageName"
		  " FROM tst_questions,tst_answers"
		  " WHERE tst_questions.CrsCod='%ld'"
		  " AND tst_questions.QstCod=tst_answers.QstCod",
	    CrsCod);
   NumImages = (unsigned) DB_QuerySELECT (Query,&mysql_res,"can not get test images");

   /***** Go over result removing image files *****/
   for (NumImg = 0;
	NumImg < NumImages;
	NumImg++)
     {
      /***** Get image name (row[0]) *****/
      row = mysql_fetch_row (mysql_res);

      /***** Remove image file *****/
      Img_RemoveImageFile (row[0]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/*********************** Get stats about test questions **********************/
/*****************************************************************************/

void Tst_GetTestStats (Tst_AnswerType_t AnsType,struct Tst_Stats *Stats)
  {
   Stats->NumQsts = 0;
   Stats->NumCoursesWithQuestions = Stats->NumCoursesWithPluggableQuestions = 0;
   Stats->AvgQstsPerCourse = 0.0;
   Stats->NumHits = 0L;
   Stats->AvgHitsPerCourse = 0.0;
   Stats->AvgHitsPerQuestion = 0.0;
   Stats->TotalScore = 0.0;
   Stats->AvgScorePerQuestion = 0.0;

   if (Tst_GetNumTstQuestions (Gbl.Scope.Current,AnsType,Stats))
     {
      if ((Stats->NumCoursesWithQuestions = Tst_GetNumCoursesWithTstQuestions (Gbl.Scope.Current,AnsType)) != 0)
        {
         Stats->NumCoursesWithPluggableQuestions = Tst_GetNumCoursesWithPluggableTstQuestions (Gbl.Scope.Current,AnsType);
         Stats->AvgQstsPerCourse = (float) Stats->NumQsts / (float) Stats->NumCoursesWithQuestions;
         Stats->AvgHitsPerCourse = (float) Stats->NumHits / (float) Stats->NumCoursesWithQuestions;
        }
      Stats->AvgHitsPerQuestion = (float) Stats->NumHits / (float) Stats->NumQsts;
      if (Stats->NumHits)
         Stats->AvgScorePerQuestion = Stats->TotalScore / (double) Stats->NumHits;
     }
  }

/*****************************************************************************/
/*********************** Get number of test questions ************************/
/*****************************************************************************/
// Returns the number of test questions
// in this location (all the platform, current degree or current course)

static unsigned Tst_GetNumTstQuestions (Sco_Scope_t Scope,Tst_AnswerType_t AnsType,struct Tst_Stats *Stats)
  {
   char Query[1024];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Get number of test questions from database *****/
   switch (Scope)
     {
      case Sco_SCOPE_SYS:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM tst_questions");
         else
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM tst_questions"
                           " WHERE AnsType='%s'",
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_CTY:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM institutions,centres,degrees,courses,tst_questions"
                           " WHERE institutions.CtyCod='%ld'"
                           " AND institutions.InsCod=centres.InsCod"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod",
                     Gbl.CurrentCty.Cty.CtyCod);
         else
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM institutions,centres,degrees,courses,tst_questions"
                           " WHERE institutions.CtyCod='%ld'"
                           " AND institutions.InsCod=centres.InsCod"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'",
                     Gbl.CurrentCty.Cty.CtyCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_INS:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM centres,degrees,courses,tst_questions"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod",
                     Gbl.CurrentIns.Ins.InsCod);
         else
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM centres,degrees,courses,tst_questions"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'",
                     Gbl.CurrentIns.Ins.InsCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_CTR:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM degrees,courses,tst_questions"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod",
                     Gbl.CurrentCtr.Ctr.CtrCod);
         else
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM degrees,courses,tst_questions"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'",
                     Gbl.CurrentCtr.Ctr.CtrCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_DEG:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM courses,tst_questions"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=tst_questions.CrsCod",
                     Gbl.CurrentDeg.Deg.DegCod);
         else
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM courses,tst_questions"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'",
                     Gbl.CurrentDeg.Deg.DegCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_CRS:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM tst_questions"
                           " WHERE CrsCod='%ld'",
                     Gbl.CurrentCrs.Crs.CrsCod);
         else
            sprintf (Query,"SELECT COUNT(*),SUM(NumHits),SUM(Score)"
        	           " FROM tst_questions"
                           " WHERE CrsCod='%ld' AND AnsType='%s'",
                     Gbl.CurrentCrs.Crs.CrsCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }
   DB_QuerySELECT (Query,&mysql_res,"can not get number of test questions");

   /***** Get number of questions *****/
   row = mysql_fetch_row (mysql_res);
   if (sscanf (row[0],"%u",&(Stats->NumQsts)) != 1)
      Lay_ShowErrorAndExit ("Error when getting number of test questions.");

   if (Stats->NumQsts)
     {
      if (sscanf (row[1],"%lu",&(Stats->NumHits)) != 1)
         Lay_ShowErrorAndExit ("Error when getting total number of hits in test questions.");

      setlocale (LC_NUMERIC,"en_US.utf8");	// To get decimal point
      if (sscanf (row[2],"%lf",&(Stats->TotalScore)) != 1)
         Lay_ShowErrorAndExit ("Error when getting total score in test questions.");
      setlocale (LC_NUMERIC,"es_ES.utf8");	// Return to spanish system (TODO: this should be internationalized!!!!!!!)
     }
   else
     {
      Stats->NumHits = 0L;
      Stats->TotalScore = 0.0;
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return Stats->NumQsts;
  }

/*****************************************************************************/
/**************** Get number of courses with test questions ******************/
/*****************************************************************************/
// Returns the number of courses with test questions
// in this location (all the platform, current degree or current course)

static unsigned Tst_GetNumCoursesWithTstQuestions (Sco_Scope_t Scope,Tst_AnswerType_t AnsType)
  {
   char Query[1024];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumCourses;

   /***** Get number of courses with test questions from database *****/
   switch (Scope)
     {
      case Sco_SCOPE_SYS:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (CrsCod))"
        	           " FROM tst_questions");
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (CrsCod))"
        	           " FROM tst_questions"
                           " WHERE AnsType='%s'",
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_CTY:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM institutions,centres,degrees,courses,tst_questions"
                           " WHERE institutions.CtyCod='%ld'"
                           " AND institutions.InsCod=centres.InsCod"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod",
                     Gbl.CurrentCty.Cty.CtyCod);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM institutions,centres,degrees,courses,tst_questions"
                           " WHERE institutions.CtyCod='%ld'"
                           " AND institutions.InsCod=centres.InsCod"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'",
                     Gbl.CurrentCty.Cty.CtyCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_INS:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM centres,degrees,courses,tst_questions"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod",
                     Gbl.CurrentIns.Ins.InsCod);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM centres,degrees,courses,tst_questions"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'",
                     Gbl.CurrentIns.Ins.InsCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_CTR:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM degrees,courses,tst_questions"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod",
                     Gbl.CurrentCtr.Ctr.CtrCod);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM degrees,courses,tst_questions"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'",
                     Gbl.CurrentCtr.Ctr.CtrCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_DEG:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM courses,tst_questions"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=tst_questions.CrsCod",
                     Gbl.CurrentDeg.Deg.DegCod);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM courses,tst_questions"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'",
                     Gbl.CurrentDeg.Deg.DegCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      case Sco_SCOPE_CRS:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (CrsCod))"
        	           " FROM tst_questions"
                           " WHERE CrsCod='%ld'",
                     Gbl.CurrentCrs.Crs.CrsCod);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (CrsCod))"
        	           " FROM tst_questions"
                           " WHERE CrsCod='%ld'"
                           " AND AnsType='%s'",
                     Gbl.CurrentCrs.Crs.CrsCod,
                     Tst_StrAnswerTypesDB[AnsType]);
         break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }
   DB_QuerySELECT (Query,&mysql_res,"can not get number of courses with test questions");

   /***** Get number of courses *****/
   row = mysql_fetch_row (mysql_res);
   if (sscanf (row[0],"%u",&NumCourses) != 1)
      Lay_ShowErrorAndExit ("Error when getting number of courses with test questions.");

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return NumCourses;
  }

/*****************************************************************************/
/*********** Get number of courses with pluggable test questions *************/
/*****************************************************************************/
// Returns the number of courses with pluggable test questions
// in this location (all the platform, current degree or current course)

static unsigned Tst_GetNumCoursesWithPluggableTstQuestions (Sco_Scope_t Scope,Tst_AnswerType_t AnsType)
  {
   char Query[1024];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumCourses;

   /***** Get number of courses with test questions from database *****/
   switch (Scope)
     {
      case Sco_SCOPE_SYS:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT tst_questions.CrsCod)"
        	           " FROM tst_questions,tst_config"
                           " WHERE tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT tst_questions.CrsCod)"
        	           " FROM tst_questions,tst_config"
                           " WHERE tst_questions.AnsType='%s'"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Tst_StrAnswerTypesDB[AnsType],
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         break;
      case Sco_SCOPE_CTY:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM institutions,centres,degrees,courses,tst_questions,tst_config"
                           " WHERE institutions.CtyCod='%ld'"
                           " AND institutions.InsCod=centres.InsCod"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentCty.Cty.CtyCod,
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM institutions,centres,degrees,courses,tst_questions,tst_config"
                           " WHERE institutions.CtyCod='%ld'"
                           " AND institutions.InsCod=centres.InsCod"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentCty.Cty.CtyCod,
                     Tst_StrAnswerTypesDB[AnsType],
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         break;
      case Sco_SCOPE_INS:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM centres,degrees,courses,tst_questions,tst_config"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentIns.Ins.InsCod,
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM centres,degrees,courses,tst_questions,tst_config"
                           " WHERE centres.InsCod='%ld'"
                           " AND centres.CtrCod=degrees.CtrCod"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentIns.Ins.InsCod,
                     Tst_StrAnswerTypesDB[AnsType],
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         break;
      case Sco_SCOPE_CTR:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM degrees,courses,tst_questions,tst_config"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentCtr.Ctr.CtrCod,
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM degrees,courses,tst_questions,tst_config"
                           " WHERE degrees.CtrCod='%ld'"
                           " AND degrees.DegCod=courses.DegCod"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentCtr.Ctr.CtrCod,
                     Tst_StrAnswerTypesDB[AnsType],
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         break;
      case Sco_SCOPE_DEG:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM courses,tst_questions,tst_config"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentDeg.Deg.DegCod,
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT (tst_questions.CrsCod))"
        	           " FROM courses,tst_questions,tst_config"
                           " WHERE courses.DegCod='%ld'"
                           " AND courses.CrsCod=tst_questions.CrsCod"
                           " AND tst_questions.AnsType='%s'"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentDeg.Deg.DegCod,
                     Tst_StrAnswerTypesDB[AnsType],
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         break;
      case Sco_SCOPE_CRS:
         if (AnsType == Tst_ANS_ALL)
            sprintf (Query,"SELECT COUNT(DISTINCT tst_questions.CrsCod)"
        	           " FROM tst_questions,tst_config"
                           " WHERE tst_questions.CrsCod='%ld'"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentCrs.Crs.CrsCod,
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         else
            sprintf (Query,"SELECT COUNT(DISTINCT tst_questions.CrsCod)"
        	           " FROM tst_questions,tst_config"
                           " WHERE tst_questions.CrsCod='%ld'"
                           " AND tst_questions.AnsType='%s'"
                           " AND tst_questions.CrsCod=tst_config.CrsCod"
                           " AND tst_config.pluggable='%s'",
                     Gbl.CurrentCrs.Crs.CrsCod,
                     Tst_StrAnswerTypesDB[AnsType],
                     Tst_PluggableDB[Tst_PLUGGABLE_YES]);
         break;
      default:
	 Lay_ShowErrorAndExit ("Wrong scope.");
	 break;
     }
   DB_QuerySELECT (Query,&mysql_res,"can not get number of courses with pluggable test questions");

   /***** Get number of courses *****/
   row = mysql_fetch_row (mysql_res);
   if (sscanf (row[0],"%u",&NumCourses) != 1)
      Lay_ShowErrorAndExit ("Error when getting number of courses with pluggable test questions.");

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   return NumCourses;
  }

/*****************************************************************************/
/******* Select users and dates to show their results in test exams **********/
/*****************************************************************************/

void Tst_SelUsrsToSeeUsrsTstExams (void)
  {
   extern const char *The_ClassForm[The_NUM_THEMES];
   extern const char *Txt_Exams;
   extern const char *Txt_Users;
   extern const char *Txt_See_exams;

   /***** Get and update type of list,
          number of columns in class photo
          and preference about view photos *****/
   Usr_GetAndUpdatePrefsAboutUsrList ();

   /***** Show form to select the groups *****/
   Grp_ShowFormToSelectSeveralGroups (ActReqSeeUsrTstExa);

   /***** Get and order lists of users from this course *****/
   Usr_GetUsrsLst (Rol_TEACHER,Sco_SCOPE_CRS,NULL,false);
   Usr_GetUsrsLst (Rol_STUDENT,Sco_SCOPE_CRS,NULL,false);

   if (Gbl.Usrs.LstTchs.NumUsrs ||
       Gbl.Usrs.LstStds.NumUsrs)
     {
      if (Usr_GetIfShowBigList (Gbl.Usrs.LstTchs.NumUsrs +
	                        Gbl.Usrs.LstStds.NumUsrs))
        {
         /***** Start frame *****/
         Lay_StartRoundFrame (NULL,Txt_Exams,NULL);

	 /***** Form to select type of list used for select several users *****/
	 Usr_ShowFormsToSelectUsrListType (ActReqSeeUsrTstExa);

         /***** Start form *****/
         Act_FormStart (ActSeeUsrTstExa);
         Grp_PutParamsCodGrps ();

         /***** Put list of users to select some of them *****/
         fprintf (Gbl.F.Out,"<table class=\"CELLS_PAD_2\""
                            " style=\"margin:0 auto;\">"
                            "<tr>"
			    "<td class=\"%s RIGHT_TOP\">"
			    "%s:"
			    "</td>"
			    "<td colspan=\"2\" class=\"%s LEFT_TOP\">"
                            "<table class=\"CELLS_PAD_2\">",
                  The_ClassForm[Gbl.Prefs.Theme],Txt_Users,
                  The_ClassForm[Gbl.Prefs.Theme]);
         Usr_ListUsersToSelect (Rol_TEACHER);
         Usr_ListUsersToSelect (Rol_STUDENT);
         fprintf (Gbl.F.Out,"</table>"
                            "</td>"
                            "</tr>");

         /***** Starting and ending dates in the search *****/
         Dat_PutFormStartEndClientLocalDateTimesWithYesterdayToday ();

         fprintf (Gbl.F.Out,"</table>");

         /***** Send button *****/
	 Lay_PutConfirmButton (Txt_See_exams);

         /***** End form *****/
         Act_FormEnd ();

         /***** End frame *****/
         Lay_EndRoundFrame ();
        }
     }
   else
      Usr_ShowWarningNoUsersFound (Rol_UNKNOWN);

   /***** Free memory for users' list *****/
   Usr_FreeUsrsList (&Gbl.Usrs.LstTchs);
   Usr_FreeUsrsList (&Gbl.Usrs.LstStds);

   /***** Free the memory used by the list of users *****/
   Usr_FreeListsSelectedUsrCods ();

   /***** Free memory for list of selected groups *****/
   Grp_FreeListCodSelectedGrps ();
  }

/*****************************************************************************/
/************** Select dates to show my results in test exams ****************/
/*****************************************************************************/

void Tst_SelDatesToSeeMyTstExams (void)
  {
   extern const char *Txt_Exams;
   extern const char *Txt_See_exams;

   /***** Start form *****/
   Act_FormStart (ActSeeMyTstExa);

   /***** Starting and ending dates in the search *****/
   Lay_StartRoundFrameTable (NULL,2,Txt_Exams);
   Dat_PutFormStartEndClientLocalDateTimesWithYesterdayToday ();

   /***** Send button and end frame *****/
   Lay_EndRoundFrameTableWithButton (Lay_CONFIRM_BUTTON,Txt_See_exams);

   /***** End form *****/
   Act_FormEnd ();
  }

/*****************************************************************************/
/********************* Store test result in database *************************/
/*****************************************************************************/

static long Tst_CreateTestExamInDB (void)
  {
   char Query[256];

   /***** Insert new tag into tst_tags table *****/
   sprintf (Query,"INSERT INTO tst_exams (CrsCod,UsrCod,AllowTeachers,TstTime,NumQsts)"
                  " VALUES ('%ld','%ld','%c',NOW(),'%u')",
            Gbl.CurrentCrs.Crs.CrsCod,
            Gbl.Usrs.Me.UsrDat.UsrCod,
            Gbl.Test.AllowTeachers ? 'Y' :
        	                     'N',
            Gbl.Test.NumQsts);
   return DB_QueryINSERTandReturnCode (Query,"can not create new test exam");
  }

/*****************************************************************************/
/********************* Store test result in database *************************/
/*****************************************************************************/

static void Tst_StoreScoreOfTestExamInDB (long TstCod,
                                          unsigned NumQstsNotBlank,double Score)
  {
   char Query[256];

   /***** Update score in test exam *****/
   setlocale (LC_NUMERIC,"en_US.utf8");	// To print the floating point as a dot
   sprintf (Query,"UPDATE tst_exams"
	          " SET NumQstsNotBlank='%u',Score='%lf'"
	          " WHERE TstCod='%ld'",
            NumQstsNotBlank,Score,
            TstCod);
   setlocale (LC_NUMERIC,"es_ES.utf8");	// Return to spanish system (TODO: this should be internationalized!!!!!!!)
   DB_QueryUPDATE (Query,"can not update result of test exam");
  }

/*****************************************************************************/
/*************** Show results in test exams for several users ****************/
/*****************************************************************************/

void Tst_ShowUsrsTestResults (void)
  {
   extern const char *Txt_Exams;
   extern const char *Txt_You_must_select_one_ore_more_users;
   const char *Ptr;

   /***** Get list of the selected users's IDs *****/
   Usr_GetListsSelectedUsrs ();

   /***** Get starting and ending dates *****/
   Dat_GetIniEndDatesFromForm ();

   /***** Check the number of users whose tests results will be shown *****/
   if (Usr_CountNumUsrsInListOfSelectedUsrs ())				// If some users are selected...
     {
      /***** Header of the table with the list of users *****/
      Lay_StartRoundFrameTable (NULL,2,Txt_Exams);
      Tst_ShowHeaderTestResults ();

      /***** List the assignments and works of the selected users *****/
      Ptr = Gbl.Usrs.Select.All;
      while (*Ptr)
	{
	 Par_GetNextStrUntilSeparParamMult (&Ptr,Gbl.Usrs.Other.UsrDat.EncryptedUsrCod,Cry_LENGTH_ENCRYPTED_STR_SHA256_BASE64);
	 Usr_GetUsrCodFromEncryptedUsrCod (&Gbl.Usrs.Other.UsrDat);
	 if (Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat))               // Get of the database the data of the user
	    if (Usr_CheckIfUsrBelongsToCrs (Gbl.Usrs.Other.UsrDat.UsrCod,
	                                    Gbl.CurrentCrs.Crs.CrsCod,
	                                    false))
	       /***** Show the result of test exams *****/
	       Tst_ShowResultsOfTestExams (&Gbl.Usrs.Other.UsrDat);
	}

      /***** End of the table *****/
      Lay_EndRoundFrameTable ();
     }
   else	// If no users are selected...
     {
      // ...write warning alert
      Lay_ShowAlert (Lay_WARNING,Txt_You_must_select_one_ore_more_users);
      // ...and show again the form
      Tst_SelUsrsToSeeUsrsTstExams ();
     }

   /***** Free the memory used for the list of users *****/
   Usr_FreeListsSelectedUsrCods ();
  }

/*****************************************************************************/
/************************ Show my results in test exams **********************/
/*****************************************************************************/

static void Tst_ShowHeaderTestResults (void)
  {
   extern const char *Txt_User;
   extern const char *Txt_Date;
   extern const char *Txt_Questions;
   extern const char *Txt_Non_blank_BR_questions;
   extern const char *Txt_Total_BR_score;
   extern const char *Txt_Average_BR_score_BR_per_question_BR_from_0_to_1;
   extern const char *Txt_Score;
   extern const char *Txt_out_of_PART_OF_A_SCORE;

   fprintf (Gbl.F.Out,"<tr>"
		      "<th colspan=\"2\" class=\"CENTER_TOP\">"
		      "%s"
		      "</th>"
		      "<th class=\"RIGHT_TOP\">"
		      "%s"
		      "</th>"
		      "<th class=\"RIGHT_TOP\">"
		      "%s"
		      "</th>"
		      "<th class=\"RIGHT_TOP\">"
		      "%s"
		      "</th>"
		      "<th class=\"RIGHT_TOP\">"
		      "%s"
		      "</th>"
		      "<th class=\"RIGHT_TOP\">"
		      "%s"
		      "</th>"
		      "<th class=\"RIGHT_TOP\">"
		      "%s<br />%s<br />%u"
		      "</th>"
		      "<th></th>"
		      "</tr>",
	    Txt_User,
	    Txt_Date,
	    Txt_Questions,
	    Txt_Non_blank_BR_questions,
	    Txt_Total_BR_score,
	    Txt_Average_BR_score_BR_per_question_BR_from_0_to_1,
	    Txt_Score,Txt_out_of_PART_OF_A_SCORE,Tst_SCORE_MAX);
  }

/*****************************************************************************/
/************************ Show my results in test exams **********************/
/*****************************************************************************/

void Tst_ShowMyTestResults (void)
  {
   extern const char *Txt_Exams;

   /***** Get starting and ending dates *****/
   Dat_GetIniEndDatesFromForm ();

   /***** Header of the table with the list of users *****/
   Lay_StartRoundFrameTable (NULL,2,Txt_Exams);
   Tst_ShowHeaderTestResults ();

   /***** List my results in test exams *****/
   Tst_GetConfigTstFromDB ();	// To get feedback type
   Tst_ShowResultsOfTestExams (&Gbl.Usrs.Me.UsrDat);

   /***** End of the table *****/
   Lay_EndRoundFrameTable ();
  }

/*****************************************************************************/
/****** Show the results of test exams of a user in the current course *******/
/*****************************************************************************/

static void Tst_ShowResultsOfTestExams (struct UsrData *UsrDat)
  {
   extern const char *Txt_Today;
   extern const char *Txt_Visible_exams;
   extern const char *Txt_See_exam;
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumExams;
   unsigned NumExam;
   static unsigned UniqueId = 0;
   long TstCod;
   unsigned NumQstsInThisExam;
   unsigned NumQstsNotBlankInThisExam;
   unsigned NumTotalQsts = 0;
   unsigned NumTotalQstsNotBlank = 0;
   double ScoreInThisExam;
   double TotalScoreOfAllExams = 0.0;
   unsigned NumExamsVisibleByTchs = 0;
   bool ItsMe = (UsrDat->UsrCod == Gbl.Usrs.Me.UsrDat.UsrCod);
   bool IAmATeacher = (Gbl.Usrs.Me.LoggedRole >= Rol_TEACHER);
   bool ICanViewExam;
   bool ICanViewScore;
   time_t TimeUTC;
   char *ClassDat;

   /***** Make database query *****/
   sprintf (Query,"SELECT TstCod,AllowTeachers,"
	          "UNIX_TIMESTAMP(TstTime),"
	          "NumQsts,NumQstsNotBlank,Score"
	          " FROM tst_exams"
                  " WHERE CrsCod='%ld' AND UsrCod='%ld'"
                  " AND TstTime>=FROM_UNIXTIME('%ld')"
                  " AND TstTime<=FROM_UNIXTIME('%ld')"
                  " ORDER BY TstCod",
            Gbl.CurrentCrs.Crs.CrsCod,
            UsrDat->UsrCod,
            (long) Gbl.DateRange.TimeUTC[0],
            (long) Gbl.DateRange.TimeUTC[1]);
   NumExams = (unsigned) DB_QuerySELECT (Query,&mysql_res,"can not get test exams of a user");

   /***** Show user's data *****/
   fprintf (Gbl.F.Out,"<tr>");
   Tst_ShowDataUsr (UsrDat,NumExams);

   /***** Get and print exams results *****/
   if (NumExams)
     {
      for (NumExam = 0;
           NumExam < NumExams;
           NumExam++)
        {
         row = mysql_fetch_row (mysql_res);

         /* Get test code (row[0]) */
	 if ((TstCod = Str_ConvertStrCodToLongCod (row[0])) < 0)
	    Lay_ShowErrorAndExit ("Wrong code of test exam.");

	 /* Get if teachers are allowed to see this test exam (row[1]) */
	 Gbl.Test.AllowTeachers = (Str_ConvertToUpperLetter (row[1][0]) == 'Y');
	 ClassDat = Gbl.Test.AllowTeachers ? "DAT" :
	                                     "DAT_LIGHT";
	 ICanViewExam = ItsMe || Gbl.Test.AllowTeachers;
	 ICanViewScore = ICanViewExam && (IAmATeacher ||
	                                  Gbl.Test.Config.FeedbackType != Tst_FEEDBACK_NOTHING);

         if (NumExam)
            fprintf (Gbl.F.Out,"<tr>");

         /* Write date and time (row[2] holds UTC date-time) */
         TimeUTC = Dat_GetUNIXTimeFromStr (row[2]);
         UniqueId++;
	 fprintf (Gbl.F.Out,"<td id =\"tst_date_%u\" class=\"%s RIGHT_TOP COLOR%u\">"
			    "<script type=\"text/javascript\">"
			    "writeLocalDateHMSFromUTC('tst_date_%u',%ld,'&nbsp;','%s');"
			    "</script>"
			    "</td>",
	          UniqueId,ClassDat,Gbl.RowEvenOdd,
	          UniqueId,(long) TimeUTC,Txt_Today);

         /* Get number of questions (row[3]) */
         if (sscanf (row[3],"%u",&NumQstsInThisExam) == 1)
           {
            if (Gbl.Test.AllowTeachers)
               NumTotalQsts += NumQstsInThisExam;
           }
         else
            NumQstsInThisExam = 0;

         /* Get number of questions not blank (row[4]) */
         if (sscanf (row[4],"%u",&NumQstsNotBlankInThisExam) == 1)
           {
            if (Gbl.Test.AllowTeachers)
               NumTotalQstsNotBlank += NumQstsNotBlankInThisExam;
           }
         else
            NumQstsNotBlankInThisExam = 0;

         /* Get score (row[5]) */
         setlocale (LC_NUMERIC,"en_US.utf8");	// To get decimal point
         if (sscanf (row[5],"%lf",&ScoreInThisExam) == 1)
           {
            if (Gbl.Test.AllowTeachers)
               TotalScoreOfAllExams += ScoreInThisExam;
           }
         else
            ScoreInThisExam = 0.0;
         setlocale (LC_NUMERIC,"es_ES.utf8");	// Return to spanish system (TODO: this should be internationalized!!!!!!!)

         /* Write number of questions */
	 fprintf (Gbl.F.Out,"<td class=\"%s RIGHT_TOP COLOR%u\">",
	          ClassDat,Gbl.RowEvenOdd);
	 if (ICanViewExam)
	    fprintf (Gbl.F.Out,"%u",NumQstsInThisExam);
	 fprintf (Gbl.F.Out,"</td>");

         /* Write number of questions not blank */
	 fprintf (Gbl.F.Out,"<td class=\"%s RIGHT_TOP COLOR%u\">",
	          ClassDat,Gbl.RowEvenOdd);
	 if (ICanViewExam)
	    fprintf (Gbl.F.Out,"%u",NumQstsNotBlankInThisExam);
	 fprintf (Gbl.F.Out,"</td>");

	 /* Write score */
	 fprintf (Gbl.F.Out,"<td class=\"%s RIGHT_TOP COLOR%u\">",
	          ClassDat,Gbl.RowEvenOdd);
	 if (ICanViewScore)
	    fprintf (Gbl.F.Out,"%.2lf",ScoreInThisExam);
	 fprintf (Gbl.F.Out,"</td>");

         /* Write average score per question */
	 fprintf (Gbl.F.Out,"<td class=\"%s RIGHT_TOP COLOR%u\">",
	          ClassDat,Gbl.RowEvenOdd);
	 if (ICanViewScore)
	    fprintf (Gbl.F.Out,"%.2lf",
		     NumQstsInThisExam ? ScoreInThisExam / (double) NumQstsInThisExam :
			                 0.0);
	 fprintf (Gbl.F.Out,"</td>");

         /* Write score over Tst_SCORE_MAX */
	 fprintf (Gbl.F.Out,"<td class=\"%s RIGHT_TOP COLOR%u\">",
	          ClassDat,Gbl.RowEvenOdd);
	 if (ICanViewScore)
	    fprintf (Gbl.F.Out,"%.2lf",
		     NumQstsInThisExam ? ScoreInThisExam * Tst_SCORE_MAX / (double) NumQstsInThisExam :
			                 0.0);
	 fprintf (Gbl.F.Out,"</td>");

	 /* Link to show this exam */
	 fprintf (Gbl.F.Out,"<td class=\"RIGHT_TOP COLOR%u\">",
		  Gbl.RowEvenOdd);
	 if (ICanViewExam)
	   {
	    Act_FormStart (Gbl.Action.Act == ActSeeMyTstExa ? ActSeeOneTstExaMe :
						              ActSeeOneTstExaOth);
	    Tst_PutParamTstCod (TstCod);
	    fprintf (Gbl.F.Out,"<input type=\"image\" src=\"%s/file64x64.gif\""
			       " alt=\"%s\" title=\"%s\""
			       " class=\"ICON20x20B\" />",
		     Gbl.Prefs.IconsURL,
		     Txt_See_exam,
		     Txt_See_exam);
	    Act_FormEnd ();
	   }
	 fprintf (Gbl.F.Out,"</td>"
                            "</tr>");

	 if (Gbl.Test.AllowTeachers)
            NumExamsVisibleByTchs++;
        }

      /***** Write totals for this user *****/
      ICanViewScore = NumExamsVisibleByTchs && (IAmATeacher ||
	                                        Gbl.Test.Config.FeedbackType != Tst_FEEDBACK_NOTHING);

      fprintf (Gbl.F.Out,"<tr>"
			 "<td class=\"DAT_N_LINE_TOP RIGHT_MIDDLE COLOR%u\">"
			 "%s: %u"
			 "</td>",
	       Gbl.RowEvenOdd,
	       Txt_Visible_exams,NumExamsVisibleByTchs);

      /* Write total number of questions */
      fprintf (Gbl.F.Out,"<td class=\"DAT_N_LINE_TOP RIGHT_MIDDLE COLOR%u\">",
	       Gbl.RowEvenOdd);
      if (NumExamsVisibleByTchs)
         fprintf (Gbl.F.Out,"%u",NumTotalQsts);
      fprintf (Gbl.F.Out,"</td>");

      /* Write total number of questions not blank */
      fprintf (Gbl.F.Out,"<td class=\"DAT_N_LINE_TOP RIGHT_MIDDLE COLOR%u\">",
	       Gbl.RowEvenOdd);
      if (NumExamsVisibleByTchs)
         fprintf (Gbl.F.Out,"%u",NumTotalQstsNotBlank);
      fprintf (Gbl.F.Out,"</td>");

      /* Write total score */
      fprintf (Gbl.F.Out,"<td class=\"DAT_N_LINE_TOP RIGHT_MIDDLE COLOR%u\">",
	       Gbl.RowEvenOdd);
      if (ICanViewScore)
         fprintf (Gbl.F.Out,"%.2lf",TotalScoreOfAllExams);
      fprintf (Gbl.F.Out,"</td>");

      /* Write average score per question */
      fprintf (Gbl.F.Out,"<td class=\"DAT_N_LINE_TOP RIGHT_MIDDLE COLOR%u\">",
	       Gbl.RowEvenOdd);
      if (ICanViewScore)
         fprintf (Gbl.F.Out,"%.2lf",
                  NumTotalQsts ? TotalScoreOfAllExams / (double) NumTotalQsts :
                	         0.0);
      fprintf (Gbl.F.Out,"</td>");

      /* Write score over Tst_SCORE_MAX */
      fprintf (Gbl.F.Out,"<td class=\"DAT_N_LINE_TOP RIGHT_MIDDLE COLOR%u\">",
	       Gbl.RowEvenOdd);
      if (ICanViewScore)
         fprintf (Gbl.F.Out,"%.2lf",
                  NumTotalQsts ? TotalScoreOfAllExams * Tst_SCORE_MAX / (double) NumTotalQsts :
                	         0.0);
      fprintf (Gbl.F.Out,"</td>");

      /* Last cell */
      fprintf (Gbl.F.Out,"<td class=\"DAT_N_LINE_TOP COLOR%u\"></td>"
			 "</tr>",
	       Gbl.RowEvenOdd);
     }
   else
      fprintf (Gbl.F.Out,"<td class=\"COLOR%u\"></td>"
	                 "<td class=\"COLOR%u\"></td>"
	                 "<td class=\"COLOR%u\"></td>"
	                 "<td class=\"COLOR%u\"></td>"
	                 "<td class=\"COLOR%u\"></td>"
	                 "<td class=\"COLOR%u\"></td>"
	                 "<td class=\"COLOR%u\"></td>"
	                 "</tr>",
	       Gbl.RowEvenOdd,
	       Gbl.RowEvenOdd,
	       Gbl.RowEvenOdd,
	       Gbl.RowEvenOdd,
	       Gbl.RowEvenOdd,
	       Gbl.RowEvenOdd,
	       Gbl.RowEvenOdd);

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);

   Gbl.RowEvenOdd = 1 - Gbl.RowEvenOdd;
  }

/*****************************************************************************/
/******************** Show a row with the data of a user *********************/
/*****************************************************************************/

static void Tst_ShowDataUsr (struct UsrData *UsrDat,unsigned NumExams)
  {
   bool ShowPhoto;
   char PhotoURL[PATH_MAX+1];

   /***** Show user's photo and name *****/
   fprintf (Gbl.F.Out,"<td ");
   if (NumExams)
      fprintf (Gbl.F.Out,"rowspan=\"%u\"",NumExams + 1);
   fprintf (Gbl.F.Out," class=\"LEFT_TOP COLOR%u\">",
	    Gbl.RowEvenOdd);
   ShowPhoto = Pho_ShowUsrPhotoIsAllowed (UsrDat,PhotoURL);
   Pho_ShowUsrPhoto (UsrDat,ShowPhoto ? PhotoURL :
                	                NULL,
                     "PHOTO45x60",Pho_ZOOM,false);
   fprintf (Gbl.F.Out,"</td>");

   /***** Start form to go to user's record card *****/
   fprintf (Gbl.F.Out,"<td ");
   if (NumExams)
      fprintf (Gbl.F.Out,"rowspan=\"%u\"",NumExams + 1);
   fprintf (Gbl.F.Out," class=\"LEFT_TOP COLOR%u\">",
	    Gbl.RowEvenOdd);
   Act_FormStart (UsrDat->RoleInCurrentCrsDB == Rol_STUDENT ? ActSeeRecOneStd :
	                                                      ActSeeRecOneTch);
   Usr_PutParamUsrCodEncrypted (UsrDat->EncryptedUsrCod);
   Act_LinkFormSubmit (UsrDat->FullName,"MSG_AUT");

   /***** Show user's ID *****/
   ID_WriteUsrIDs (UsrDat,
                   (UsrDat->RoleInCurrentCrsDB == Rol_TEACHER ? ID_ICanSeeUsrID (UsrDat) :
                                                                     (Gbl.Usrs.Me.LoggedRole >= Rol_TEACHER)));

   /***** Show user's name *****/
   fprintf (Gbl.F.Out,"<br />%s",UsrDat->Surname1);
   if (UsrDat->Surname2[0])
      fprintf (Gbl.F.Out," %s",UsrDat->Surname2);
   if (UsrDat->FirstName[0])
      fprintf (Gbl.F.Out,",<br />%s",UsrDat->FirstName);

   /***** End form *****/
   Act_FormEnd ();
   fprintf (Gbl.F.Out,"</td>");
  }

/*****************************************************************************/
/***************** Write parameter with code of test exam ********************/
/*****************************************************************************/

static void Tst_PutParamTstCod (long TstCod)
  {
   Par_PutHiddenParamLong ("TstCod",TstCod);
  }

/*****************************************************************************/
/****************** Get parameter with code of test exam *********************/
/*****************************************************************************/

static long Tst_GetParamTstCod (void)
  {
   char LongStr[1+10+1];

   /***** Get parameter with code of test exam *****/
   Par_GetParToText ("TstCod",LongStr,1+10);
   return Str_ConvertStrCodToLongCod (LongStr);
  }

/*****************************************************************************/
/******************* Show one test exam of another user **********************/
/*****************************************************************************/

void Tst_ShowOneTestExam (void)
  {
   extern const char *Txt_Test_result;
   extern const char *Txt_ROLES_SINGUL_Abc[Rol_NUM_ROLES][Usr_NUM_SEXS];
   extern const char *Txt_Date;
   extern const char *Txt_Today;
   extern const char *Txt_Questions;
   extern const char *Txt_non_blank_QUESTIONS;
   extern const char *Txt_Score;
   extern const char *Txt_out_of_PART_OF_A_SCORE;
   extern const char *Txt_Tags;
   long TstCod;
   time_t TstTimeUTC = 0;	// Test exam UTC date-time, initialized to avoid warning
   unsigned NumQstsNotBlank;
   double TotalScore;
   bool ShowPhoto;
   char PhotoURL[PATH_MAX+1];
   bool ICanViewScore;

   /***** Get the code of the test exam *****/
   if ((TstCod = Tst_GetParamTstCod ()) == -1L)
      Lay_ShowErrorAndExit ("Code of test exam is missing.");

   /***** Get exam data and check if I can view this test exam) *****/
   Tst_GetExamDataByTstCod (TstCod,&TstTimeUTC,&Gbl.Test.NumQsts,&NumQstsNotBlank,&TotalScore);
   Gbl.Test.Config.FeedbackType = Tst_FEEDBACK_FULL_FEEDBACK;   // Initialize feedback to maximum
   ICanViewScore = true;
   switch (Gbl.Action.Act)
     {
      case ActSeeOneTstExaMe:
	 if (Gbl.Usrs.Other.UsrDat.UsrCod != Gbl.Usrs.Me.UsrDat.UsrCod)		// The exam is not mine
	    Lay_ShowErrorAndExit ("You can not view this test exam.");
	 if (Gbl.Usrs.Me.LoggedRole < Rol_TEACHER)
	   {
	    // Students only can view score if feedback type allows it
            Tst_GetConfigTstFromDB ();	// To get feedback type
            ICanViewScore = (Gbl.Test.Config.FeedbackType != Tst_FEEDBACK_NOTHING);
	   }
	 break;
      case ActSeeOneTstExaOth:
	 if (Gbl.Usrs.Other.UsrDat.UsrCod != Gbl.Usrs.Me.UsrDat.UsrCod &&	// The exam is not mine
	     !Gbl.Test.AllowTeachers)						// I am not allowed to see this exam
	    Lay_ShowErrorAndExit ("You can not view this test exam.");
	 break;
      default:	// Not applicable here
	 return;
     }

   /***** Get questions and user's answers of the test exam from database *****/
   Tst_GetExamQuestionsFromDB (TstCod);

   /***** Start frame *****/
   Lay_StartRoundFrame (NULL,Txt_Test_result,NULL);
   Lay_WriteHeaderClassPhoto (false,false,
                              Gbl.CurrentIns.Ins.InsCod,
                              Gbl.CurrentDeg.Deg.DegCod,
                              Gbl.CurrentCrs.Crs.CrsCod);

   /***** Start table *****/
   fprintf (Gbl.F.Out,"<table class=\"FRAME_TABLE CELLS_PAD_10\">");

   /***** Header row *****/
   /* Get data of the user who made the exam */
   if (!Usr_ChkUsrCodAndGetAllUsrDataFromUsrCod (&Gbl.Usrs.Other.UsrDat))
      Lay_ShowErrorAndExit ("User does not exists.");

   /* User */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"DAT_N RIGHT_TOP\">"
                      "%s:"
                      "</td>"
	              "<td class=\"DAT LEFT_TOP\">",
            Txt_ROLES_SINGUL_Abc[Gbl.Usrs.Other.UsrDat.RoleInCurrentCrsDB][Gbl.Usrs.Other.UsrDat.Sex]);
   ID_WriteUsrIDs (&Gbl.Usrs.Other.UsrDat,true);
   fprintf (Gbl.F.Out," %s",
            Gbl.Usrs.Other.UsrDat.Surname1);
   if (Gbl.Usrs.Other.UsrDat.Surname2[0])
      fprintf (Gbl.F.Out," %s",
	       Gbl.Usrs.Other.UsrDat.Surname2);
   if (Gbl.Usrs.Other.UsrDat.FirstName[0])
      fprintf (Gbl.F.Out,", %s",
	       Gbl.Usrs.Other.UsrDat.FirstName);
   fprintf (Gbl.F.Out,"<br />");
   ShowPhoto = Pho_ShowUsrPhotoIsAllowed (&Gbl.Usrs.Other.UsrDat,PhotoURL);
   Pho_ShowUsrPhoto (&Gbl.Usrs.Other.UsrDat,ShowPhoto ? PhotoURL :
                	                                NULL,
                     "PHOTO45x60",Pho_ZOOM,false);
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /* Exam date */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"DAT_N RIGHT_TOP\">"
                      "%s:"
                      "</td>"
	              "<td id=\"exam\" class=\"DAT LEFT_TOP\">"
		      "<script type=\"text/javascript\">"
		      "writeLocalDateHMSFromUTC('exam',%ld,'&nbsp;','%s');"
		      "</script>"
                      "</td>"
	              "</tr>",
            Txt_Date,TstTimeUTC,Txt_Today);

   /* Number of questions */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"DAT_N RIGHT_TOP\">"
                      "%s:"
                      "</td>"
	              "<td class=\"DAT LEFT_TOP\">"
	              "%u (%u %s)"
                      "</td>"
	              "</tr>",
            Txt_Questions,
            Gbl.Test.NumQsts,NumQstsNotBlank,Txt_non_blank_QUESTIONS);

   /* Score */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"DAT_N RIGHT_TOP\">"
                      "%s:"
                      "</td>"
	              "<td class=\"DAT LEFT_TOP\">",
            Txt_Score);
   if (ICanViewScore)
      fprintf (Gbl.F.Out,"%.2lf (%.2lf",
	       TotalScore,
	       Gbl.Test.NumQsts ? TotalScore * Tst_SCORE_MAX / (double) Gbl.Test.NumQsts :
		                  0.0);
   else
      fprintf (Gbl.F.Out,"? (?");	// No feedback
   fprintf (Gbl.F.Out," %s %u)</td>"
	              "</tr>",
	    Txt_out_of_PART_OF_A_SCORE,Tst_SCORE_MAX);

   /* Tags present in this exam */
   fprintf (Gbl.F.Out,"<tr>"
	              "<td class=\"DAT_N RIGHT_TOP\">"
                      "%s:"
                      "</td>"
	              "<td class=\"DAT LEFT_TOP\">",
            Txt_Tags);
   Tst_ShowTstTagsPresentInAnExam (TstCod);
   fprintf (Gbl.F.Out,"</td>"
	              "</tr>");

   /***** Write answers and solutions *****/
   Tst_ShowExamTstResult (TstTimeUTC);

   /***** Write total mark of test *****/
   if (ICanViewScore)
      Tst_ShowTstTotalMark (TotalScore);

   /***** End table *****/
   fprintf (Gbl.F.Out,"</table>");

   /***** End frame *****/
   Lay_EndRoundFrame ();
  }

/*****************************************************************************/
/********************* Show the result of a test exam ************************/
/*****************************************************************************/

static void Tst_ShowExamTstResult (time_t TstTimeUTC)
  {
   extern const char *Txt_Question_modified;
   extern const char *Txt_Question_removed;
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumQst;
   long QstCod;
   double ScoreThisQst;
   bool AnswerIsNotBlank;
   bool ThisQuestionHasBeenEdited;
   time_t EditTimeUTC;

   for (NumQst = 0;
	NumQst < Gbl.Test.NumQsts;
	NumQst++)
     {
      Gbl.RowEvenOdd = NumQst % 2;

      /***** Query database *****/
      if (Tst_GetOneQuestionByCod (Gbl.Test.QstCodes[NumQst],&mysql_res))	// Question exists
	{
	 /***** Get row of the result of the query *****/
	 row = mysql_fetch_row (mysql_res);
	 /*
	 row[ 0] QstCod
	 row[ 1] UNIX_TIMESTAMP(EditTime)
	 row[ 2] AnsType
	 row[ 3] Shuffle
	 row[ 4] Stem
	 row[ 5] Feedback
	 row[ 6] ImageName
	 row[ 7] ImageTitle
	 row[ 8] NumHits
	 row[ 9] NumHitsNotBlank
	 row[10] Score
	 */
	 /***** If this question has been edited later than test exam time ==> don't show question ****/
	 EditTimeUTC = Dat_GetUNIXTimeFromStr (row[1]);
	 ThisQuestionHasBeenEdited = false;
	 // if (TstTimeUTC)
	    if (EditTimeUTC > TstTimeUTC)
	       ThisQuestionHasBeenEdited = true;

	 if (ThisQuestionHasBeenEdited)
	    /***** Question has been edited *****/
	    fprintf (Gbl.F.Out,"<tr>"
			       "<td class=\"TEST_NUM_QST RIGHT_TOP COLOR%u\">"
			       "%u"
			       "</td>"
			       "<td class=\"DAT_LIGHT LEFT_TOP COLOR%u\">"
			       "%s"
			       "</td>"
			       "</tr>",
		     Gbl.RowEvenOdd,NumQst + 1,
		     Gbl.RowEvenOdd,Txt_Question_modified);
	 else
	   {
	    /***** Get the code of question (row[0]) *****/
	    if ((QstCod = Str_ConvertStrCodToLongCod (row[0])) < 0)
	       Lay_ShowErrorAndExit ("Wrong code of question.");

	    /***** Write questions and answers *****/
	    Tst_WriteQstAndAnsExam (NumQst,QstCod,row,
				    &ScoreThisQst,	// Not used here
				    &AnswerIsNotBlank);	// Not used here
	   }
	}
      else
	 /***** Question does not exists *****/
         fprintf (Gbl.F.Out,"<tr>"
	                    "<td class=\"TEST_NUM_QST RIGHT_TOP COLOR%u\">"
	                    "%u"
	                    "</td>"
	                    "<td class=\"DAT_LIGHT LEFT_TOP COLOR%u\">"
	                    "%s"
	                    "</td>"
	                    "</tr>",
                  Gbl.RowEvenOdd,NumQst + 1,
                  Gbl.RowEvenOdd,Txt_Question_removed);

      /***** Free structure that stores the query result *****/
      DB_FreeMySQLResult (&mysql_res);
     }
  }

/*****************************************************************************/
/************ Get data of a test exam using its test exam code ***************/
/*****************************************************************************/

static void Tst_GetExamDataByTstCod (long TstCod,time_t *TstTimeUTC,
                                     unsigned *NumQsts,unsigned *NumQstsNotBlank,double *Score)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;

   /***** Make database query *****/
   sprintf (Query,"SELECT UsrCod,AllowTeachers,"
	          "UNIX_TIMESTAMP(TstTime),"
	          "NumQsts,NumQstsNotBlank,Score"
	          " FROM tst_exams"
                  " WHERE TstCod='%ld' AND CrsCod='%ld'",
            TstCod,
            Gbl.CurrentCrs.Crs.CrsCod);
   if (DB_QuerySELECT (Query,&mysql_res,"can not get data of a test exam of a user") == 1)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get user code (row[0]) */
      Gbl.Usrs.Other.UsrDat.UsrCod = Str_ConvertStrCodToLongCod (row[0]);

      /* Get if teachers are allowed to see this test exam (row[1]) */
      Gbl.Test.AllowTeachers = (Str_ConvertToUpperLetter (row[1][0]) == 'Y');

      /* Get date-time (row[2] holds UTC date-time) */
      *TstTimeUTC = Dat_GetUNIXTimeFromStr (row[2]);

      /* Get number of questions (row[3]) */
      if (sscanf (row[3],"%u",NumQsts) != 1)
	 *NumQsts = 0;

      /* Get number of questions not blank (row[4]) */
      if (sscanf (row[4],"%u",NumQstsNotBlank) != 1)
	 *NumQstsNotBlank = 0;

      /* Get score (row[5]) */
      setlocale (LC_NUMERIC,"en_US.utf8");	// To get decimal point
      if (sscanf (row[5],"%lf",Score) != 1)
	 *Score = 0.0;
      setlocale (LC_NUMERIC,"es_ES.utf8");	// Return to spanish system (TODO: this should be internationalized!!!!!!!)
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/************** Store user's answers of an exam into database ****************/
/*****************************************************************************/

static void Tst_StoreOneExamQstInDB (long TstCod,long QstCod,unsigned NumQst,double Score)
  {
   char Query[256+Tst_MAX_SIZE_INDEXES_ONE_QST+Tst_MAX_SIZE_ANSWERS_ONE_QST];
   char Indexes[Tst_MAX_SIZE_INDEXES_ONE_QST+1];
   char Answers[Tst_MAX_SIZE_ANSWERS_ONE_QST+1];

   /***** Replace each separator of multiple parameters by a comma *****/
   /* In database commas are used as separators instead of special chars */
   Par_ReplaceSeparatorMultipleByComma (Gbl.Test.StrIndexesOneQst[NumQst],Indexes);
   Par_ReplaceSeparatorMultipleByComma (Gbl.Test.StrAnswersOneQst[NumQst],Answers);

   /***** Insert question and user's answers into database *****/
   setlocale (LC_NUMERIC,"en_US.utf8");	// To print the floating point as a dot
   sprintf (Query,"INSERT INTO tst_exam_questions"
		  " (TstCod,QstCod,QstInd,Score,Indexes,Answers)"
		  " VALUES ('%ld','%ld','%u','%lf','%s','%s')",
	    TstCod,QstCod,
	    NumQst,	// 0, 1, 2, 3...
	    Score,
	    Indexes,
	    Answers);
   setlocale (LC_NUMERIC,"es_ES.utf8");	// Return to spanish system (TODO: this should be internationalized!!!!!!!)
   DB_QueryINSERT (Query,"can not insert a question of an exam");
  }

/*****************************************************************************/
/************* Get the questions of a test exam from database ****************/
/*****************************************************************************/

static void Tst_GetExamQuestionsFromDB (long TstCod)
  {
   char Query[512];
   MYSQL_RES *mysql_res;
   MYSQL_ROW row;
   unsigned NumQst;

   /***** Get questions of a test exam from database *****/
   sprintf (Query,"SELECT QstCod,Indexes,Answers"
	          " FROM tst_exam_questions"
                  " WHERE TstCod='%ld' ORDER BY QstInd",
            TstCod);
   Gbl.Test.NumQsts = (unsigned) DB_QuerySELECT (Query,&mysql_res,"can not get questions of a test exam");

   /***** Get questions codes *****/
   for (NumQst = 0;
	NumQst < Gbl.Test.NumQsts;
	NumQst++)
     {
      row = mysql_fetch_row (mysql_res);

      /* Get question code */
      if ((Gbl.Test.QstCodes[NumQst] = Str_ConvertStrCodToLongCod (row[0])) < 0)
	 Lay_ShowErrorAndExit ("Wrong code of question.");

      /* Get indexes for this question (row[1]) */
      strncpy (Gbl.Test.StrIndexesOneQst[NumQst],row[1],Tst_MAX_SIZE_INDEXES_ONE_QST);
      Gbl.Test.StrIndexesOneQst[NumQst][Tst_MAX_SIZE_INDEXES_ONE_QST] = '\0';

      /* Get answers selected by user for this question (row[2]) */
      strncpy (Gbl.Test.StrAnswersOneQst[NumQst],row[2],Tst_MAX_SIZE_ANSWERS_ONE_QST);
      Gbl.Test.StrAnswersOneQst[NumQst][Tst_MAX_SIZE_ANSWERS_ONE_QST] = '\0';

      /* Replace each comma by a separator of multiple parameters */
      /* In database commas are used as separators instead of special chars */
      Par_ReplaceCommaBySeparatorMultiple (Gbl.Test.StrIndexesOneQst[NumQst]);
      Par_ReplaceCommaBySeparatorMultiple (Gbl.Test.StrAnswersOneQst[NumQst]);
     }

   /***** Free structure that stores the query result *****/
   DB_FreeMySQLResult (&mysql_res);
  }

/*****************************************************************************/
/********************** Remove test exams made by a user *********************/
/*****************************************************************************/

void Tst_RemoveExamsMadeByUsrInAllCrss (long UsrCod)
  {
   char Query[512];

   /***** Remove exams made by the specified user *****/
   sprintf (Query,"DELETE FROM tst_exam_questions"
	          " USING tst_exams,tst_exam_questions"
                  " WHERE tst_exams.UsrCod='%ld'"
                  " AND tst_exams.TstCod=tst_exam_questions.TstCod",
            UsrCod);
   DB_QueryDELETE (Query,"can not remove exams made by a user");

   sprintf (Query,"DELETE FROM tst_exams"
	          " WHERE UsrCod='%ld'",
	    UsrCod);
   DB_QueryDELETE (Query,"can not remove exams made by a user");
  }

/*****************************************************************************/
/*************** Remove test exams made by a user in a course ****************/
/*****************************************************************************/

void Tst_RemoveExamsMadeByUsrInCrs (long UsrCod,long CrsCod)
  {
   char Query[512];

   /***** Remove exams made by the specified user *****/
   sprintf (Query,"DELETE FROM tst_exam_questions"
	          " USING tst_exams,tst_exam_questions"
                  " WHERE tst_exams.CrsCod='%ld' AND tst_exams.UsrCod='%ld'"
                  " AND tst_exams.TstCod=tst_exam_questions.TstCod",
            CrsCod,UsrCod);
   DB_QueryDELETE (Query,"can not remove exams made by a user in a course");

   sprintf (Query,"DELETE FROM tst_exams"
	          " WHERE CrsCod='%ld' AND UsrCod='%ld'",
	    CrsCod,UsrCod);
   DB_QueryDELETE (Query,"can not remove exams made by a user in a course");
  }

/*****************************************************************************/
/******************* Remove all test exams made in a course ******************/
/*****************************************************************************/

void Tst_RemoveCrsExams (long CrsCod)
  {
   char Query[512];

   /***** Remove questions of exams made in the course *****/
   sprintf (Query,"DELETE FROM tst_exam_questions"
	          " USING tst_exams,tst_exam_questions"
                  " WHERE tst_exams.CrsCod='%ld'"
                  " AND tst_exams.TstCod=tst_exam_questions.TstCod",
            CrsCod);
   DB_QueryDELETE (Query,"can not remove exams made in a course");

   /***** Remove exams made in the course *****/
   sprintf (Query,"DELETE FROM tst_exams"
	          " WHERE CrsCod='%ld'",
	    CrsCod);
   DB_QueryDELETE (Query,"can not remove exams made in a course");
  }

/*****************************************************************************/
/******************* Remove all test exams made in a course ******************/
/*****************************************************************************/

void Tst_RemoveCrsTests (long CrsCod)
  {
   char Query[512];

   /***** Remove tests status in the course *****/
   sprintf (Query,"DELETE FROM tst_status WHERE CrsCod='%ld'",CrsCod);
   DB_QueryDELETE (Query,"can not remove status of tests of a course");

   /***** Remove test configuration of the course *****/
   sprintf (Query,"DELETE FROM tst_config WHERE CrsCod='%ld'",
	    CrsCod);
   DB_QueryDELETE (Query,"can not remove configuration of tests of a course");

   /***** Remove associations between test questions
          and test tags in the course *****/
   sprintf (Query,"DELETE FROM tst_question_tags"
	          " USING tst_questions,tst_question_tags"
                  " WHERE tst_questions.CrsCod='%ld'"
                  " AND tst_questions.QstCod=tst_question_tags.QstCod",
            CrsCod);
   DB_QueryDELETE (Query,"can not remove tags associated to questions of tests of a course");

   /***** Remove test tags in the course *****/
   sprintf (Query,"DELETE FROM tst_tags WHERE CrsCod='%ld'",
	    CrsCod);
   DB_QueryDELETE (Query,"can not remove tags of test of a course");

   /***** Remove test answers in the course *****/
   sprintf (Query,"DELETE FROM tst_answers USING tst_questions,tst_answers"
                  " WHERE tst_questions.CrsCod='%ld'"
                  " AND tst_questions.QstCod=tst_answers.QstCod",
            CrsCod);
   DB_QueryDELETE (Query,"can not remove answers of tests of a course");

   /***** Remove files with images associated
          to test questions in the course *****/
   Tst_RemoveAllImgFilesFromAnsOfAllQstsInCrs (CrsCod);
   Tst_RemoveAllImgFilesFromStemOfAllQstsInCrs (CrsCod);

   /***** Remove test questions in the course *****/
   sprintf (Query,"DELETE FROM tst_questions WHERE CrsCod='%ld'",
	    CrsCod);
   DB_QueryDELETE (Query,"can not remove test questions of a course");
  }
