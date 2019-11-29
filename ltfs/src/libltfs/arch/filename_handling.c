/*
**  %Z% %I% %W% %G% %U%
**
**  ZZ_Copyright_BEGIN
**
**
**  Licensed Materials - Property of IBM
**
**  IBM Linear Tape File System Single Drive Edition Version 2.2.0.2 for Linux and Mac OS X
**
**  Copyright IBM Corp. 2010, 2014
**
**  This file is part of the IBM Linear Tape File System Single Drive Edition for Linux and Mac OS X
**  (formally known as IBM Linear Tape File System)
**
**  The IBM Linear Tape File System Single Drive Edition for Linux and Mac OS X is free software;
**  you can redistribute it and/or modify it under the terms of the GNU Lesser
**  General Public License as published by the Free Software Foundation,
**  version 2.1 of the License.
**
**  The IBM Linear Tape File System Single Drive Edition for Linux and Mac OS X is distributed in the
**  hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
**  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**  See the GNU Lesser General Public License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License along with this library; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
**  or download the license from <http://www.gnu.org/licenses/>.
**
**
**  ZZ_Copyright_END
**
*************************************************************************************
**
** COMPONENT NAME:  IBM Linear Tape File System
**
** FILE NAME:       arch/filename_handling.c
**
** DESCRIPTION:     Implements platform-specific filename handling functions.
**
** AUTHOR:          Takashi Ashida
**                  IBM Yamato, Japan
**                  ashida@jp.ibm.com
**
*************************************************************************************
**
** Copyright (C) 2012 OSR Open Systems Resources, Inc.
** 
************************************************************************************* 
**
**  (C) Copyright 2015 - 2017 Hewlett Packard Enterprise Development LP
**  10/13/17 Added support for SNIA 2.4
**
*************************************************************************************
*/

#include "libltfs/arch/filename_handling.h"
#include "libltfs/fs.h"
#include "libltfs/pathname.h"

#ifdef HPE_mingw_BUILD
#include "arch/win/win_util.h"
#endif

#if defined(mingw_PLATFORM)
bool _replace_invalid_chars(char * file_name, bool * dosdev);
char * _generate_target_file_name(const char *prefix, const char *extension, int suffix, bool dosdev);
int _utf8_strlen(const char *s);
int _utf8_strncpy(char *t, const char *s, int n);
#endif

// HPE MD 22.09.2017 Added new struct to support SNIA 2.4 percent encoding

typedef struct percent_encoding {
    char  key;
    char  value[4];
} percentEncodingType;

// All reserved chars and the % sign (SNIA 2.4.0 sect 7.4) needed in functions below
percentEncodingType look_up_percent_encoding[] = {
   { 0x01, "%01" },{ 0x02, "%02" },{ 0x03, "%03" },{ 0x04, "%04" },{ 0x05, "%05" },{ 0x06, "%06" },{ 0x07, "%07" },
   { 0x08, "%08" },{ 0x0B, "%0B" },{ 0x0C, "%0C" },{ 0x0E, "%0E" },{ 0x0F, "%0F" },{ 0x10, "%10" },{ 0x11, "%11" },
   { 0x12, "%12" },{ 0x13, "%13" },{ 0x14, "%14" },{ 0x15, "%15" },{ 0x16, "%16" },{ 0x17, "%17" },{ 0x18, "%18" },
   { 0x19, "%19" },{ 0x1A, "%1A" },{ 0x1B, "%1B" },{ 0x1C, "%1C" },{ 0x1D, "%1D" },{ 0x1E, "%1E" },{ 0x1F, "%1F" },
   { 0x25, "%25" },{ 0x3A, "%3A" } };

//-Function------------------------------------------------------------------
// Name:        percent_decode_name
//
// Description: This function is called to decode any percent encoded name
//              whether it is an extended attribute, file name, or dir name.
//
//---------------------------------------------------------------------------
// Parameters:  name the file, dir or attribute name to be decoded.
//
//---------------------------------------------------------------------------
// Globals:     
//              
//
//---------------------------------------------------------------------------
// Returns:     
//
//---------------------------------------------------------------------------
// Notes: HPE new function to support SNIA 2.4     
//
//---------------------------------------------------------------------------
// References:  SNIA 2.4.0 spec section 7.4.
//
//--F------------------------------------------------------------------------
void percent_decode_name(char * name)
{

   char destination_name[LTFS_FILENAME_MAX * 4 + 1] = {0}; // To hold return filename may or may not be percent encoded
   bool decoding_complete = false;                         // Need to know if percent decoding has been carried out.
   
   int i, index, j = 0;
   for (i = 0; ; i++) {
       if (name[i]) {
   
           if (name[i] == 0x25) {
   
               for (index = 0; index < 30; index++) {
   
                   if ( (name[i + 1] == look_up_percent_encoding[index].value[1]) && 
                        (name[i + 2] == look_up_percent_encoding[index].value[2]) ) 
                   {
                       destination_name[j++] = look_up_percent_encoding[index].key;
                       decoding_complete = true;  // Need to skip over encoded chars in name as decoding performed
                       i = i + 2;
                       break;
                   }
               }
            } 
            if (!decoding_complete) {
                destination_name[j++] = name[i];  // No decoding needed so copy this char
            }
            else {
                decoding_complete = false;  // Reset flag for next char
            }
              
        }
       
       else
           break;
   }   
   
   strcpy(name, destination_name);
   
}

//-Function------------------------------------------------------------------
// Name:        percent_encode_name
//
// Description: This function is called to encode any file, dir or 
//              extended attribute name and to return a flag to show if 
//              encoding was needed.
//
//---------------------------------------------------------------------------
// Parameters:  name the file, dir or attribute name to be encoded.
//
//---------------------------------------------------------------------------
// Globals:     
//              
//
//---------------------------------------------------------------------------
// Returns:     bool to show if encoding has taken place.
//
//---------------------------------------------------------------------------
// Notes: HPE new function to support SNIA 2.4       
//
//---------------------------------------------------------------------------
// References:  SNIA 2.4.0 spec section 7.4
//
//--F------------------------------------------------------------------------
bool percent_encode_name(char * name)
{

   char destination_name[(LTFS_FILENAME_MAX * 4 + 1) * 3] = {0}; // To hold return filename may or may not be percent encoded
                                                                 // large enougth to hold all percent encoded characters.
   bool decoding_complete = false;                               // Need to know if percent decoding has been carried out.
   bool percentencoded = false;
     
   int i, index, j = 0;
   for (i = 0; ; i++) {
       if (name[i]) {

           for (index = 0; index < 30; index++) {

               if (name[i] == look_up_percent_encoding[index].key) {
                   destination_name[j++] = look_up_percent_encoding[index].value[0];
                   destination_name[j++] = look_up_percent_encoding[index].value[1];
                   destination_name[j++] = look_up_percent_encoding[index].value[2];
                   decoding_complete = true;  // No need to do anything further for this char as encoding performed
                   break;
               }
           }

           if (!decoding_complete) {
               destination_name[j++] = name[i];  // No encoding needed so copy this char
           }
           else {
               percentencoded = true;      // Need an entry in the index to show percent encoding
               decoding_complete = false;  // Reset flag for next char
           }

       }
       else
           break;
   }   
   
   
   // Check the size of percent encoded name and truncate it to fit into the original structure that is passed in
   // This may not happen as the filename sizes are limited by the OS but best not to rely on that.
   
   if ( ( strlen(destination_name) ) > (LTFS_FILENAME_MAX * 4) )
   {
      destination_name[LTFS_FILENAME_MAX * 4] = (char)NULL;
      strncpy(name, destination_name, (LTFS_FILENAME_MAX * 4 + 1) );
      ltfsmsg(LTFS_INFO, "17336I");
   }       
   else
   {   
      strcpy(name, destination_name);
   }   
   
   return percentencoded;

}

//-Function------------------------------------------------------------------
// Name:        update_xattr_safe_name
//
// Description: This function is called when an attribute is parsed from the 
//              xml off tape.  If the percentencoded tag is set then it takes
//              the encoded key, decodes it and sets the key field.
//
//---------------------------------------------------------------------------
// Parameters:  xattr  struct that holds info about the attribute
//
//---------------------------------------------------------------------------
// Globals:     
//              
//
//---------------------------------------------------------------------------
// Returns:     updates the key field in xattr
//
//---------------------------------------------------------------------------
// Notes: HPE new function to support SNIA 2.4      
//
//---------------------------------------------------------------------------
// References:  SNIA 2.4.0 spec section 7.4
//
//--F------------------------------------------------------------------------
void update_xattr_safe_name(struct xattr_info* xattr)
{

   char source_name[LTFS_FILENAME_MAX*4+1];      // To hold original file name
   char destination_name[LTFS_FILENAME_MAX*4+1]; // To hold return filename may or may not be percent encoded

   // Check to see if the name needs decoding.
   
   if (xattr->percentencoded) {
   
       strcpy(source_name, xattr->percent_encoded_key);
       strcpy(destination_name, source_name);
       
       percent_decode_name(destination_name);
       
       ltfsmsg(LTFS_DEBUG, "17323D", source_name);
       ltfsmsg(LTFS_DEBUG, "17324D", destination_name);
       
   } 
   else
   {
       strcpy(source_name, xattr->key);
       strcpy(destination_name, source_name);
   }

    
   // Now everything is decoded put it in the decoded key.
                  
   xattr->key = strdup(destination_name);
}   

/**
 *  Update platfrom_safe_name member in dentry
 * @param dentry dentry to update
 * @param handle_invalid_char Replace invalid chars in the name
 * if TRUE. otherwise the name is skipped withour updating
 * platform_safe_name field.
 */
void update_platform_safe_name(struct dentry* dentry, bool handle_invalid_char, struct ltfs_index *idx)
{
   // HPE MD 22.09.2017 Added to original function to allow decoding of any percent encoded names.
   
   char source_name[LTFS_FILENAME_MAX*4+1];      // To hold original file name
   char destination_name[LTFS_FILENAME_MAX*4+1]; // To hold return filename may or may not be percent encoded
   
   dentry->platform_safe_name = NULL;
   
   strcpy(source_name, dentry->name);
   strcpy(destination_name, source_name);

   // Check to see if the name needs decoding.

   if (dentry->percentencoded) {
   
       percent_decode_name(destination_name);
       
       ltfsmsg(LTFS_DEBUG, "17323D", source_name);
       ltfsmsg(LTFS_DEBUG, "17324D", destination_name);
   }    

                  
   strcpy(source_name, destination_name);  // This may or may not be actually encoded

#if defined(mingw_PLATFORM)
	bool dosdev = false;
	int suffix = 0;
	char *source_file_name_prefix, *source_file_name_extension;
	char *target_file_name;
	struct dentry *d;
	int ret;

	if (_replace_invalid_chars(source_name, &dosdev)) {
		if (! handle_invalid_char)
			return;
		suffix++;
	}

	/* Split source file name to prefix and extension */
	if (! dosdev) {
		source_file_name_prefix = source_name;
		source_file_name_extension = strrchr(source_name, '.');

		/* If '.' is at the beginning of file name, then all of file name is
		   recognized as prefix, not extension. */
		if (source_file_name_extension == source_file_name_prefix)
			source_file_name_extension = NULL;
	} else {
		source_file_name_prefix = source_name;
		source_file_name_extension = strchr(source_name, '.');
	}

	if (source_file_name_extension) {
		*source_file_name_extension = 0x00;
		source_file_name_extension++;
	}

	while (1) {
		target_file_name = _generate_target_file_name(source_file_name_prefix, source_file_name_extension, suffix, dosdev);

		if (! target_file_name)
			break;
		else {
			if (dentry->parent) {
				ret = fs_directory_lookup(dentry->parent, target_file_name, &d);
				if (ret < 0) {
					free(target_file_name);
					break;
				}
			}
			if (! dentry->parent || ! d ) {
				dentry->platform_safe_name = target_file_name;
				break;
			} else {
				if (d) {
					d->numhandles--;
				}
				suffix++;
				free(target_file_name);
			}
		}
	}
#else
	dentry->platform_safe_name = strdup(destination_name);
#endif
}

/**
 *  Perform platform dependent name matching
 * @param name1 A file name to be matched.
 * @param name2 A file name to be matched.
 * @param result Outputs matching result
 */
int ltfs_compare_names(const char *name1, const char *name2, int *result)
{
#if defined(mingw_PLATFORM)
	return pathname_caseless_match(name1, name2, result);
#else
	*result = strcmp(name1, name2);
	return 0;
#endif
}

#if defined(mingw_PLATFORM)
/**
 *  Replace invalid chars for a file name with '_'. Returns TRUE
 *  if the file name is changed in this function or the file
 *  name includes reserved dos device name.
 * @param file_name file_name to be checked
 */
bool _replace_invalid_chars(char * file_name, bool * dosdev)
{
	bool to_be_changed = false;
	static char invalid_chars[] = "\\:*?\"<>|" ;
	static char *dosdev_list[] = {	"CON", "PRN", "AUX", "CLOCK$", "NUL", "COM0",
									"COM1", "COM2", "COM3", "COM4", "COM5", "COM6",
									"COM7", "COM8", "COM9", "LPT0", "LPT1", "LPT2",
									"LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8",
									"LPT9", NULL };
	int i;

	*dosdev = false;
	for (i = 0; dosdev_list[i]; i++) {
		if ( strcasestr(file_name, dosdev_list[i]) == file_name &&
			 (file_name[strlen(dosdev_list[i])] == '\0' ||file_name[strlen(dosdev_list[i])] == '.') ) {
			*dosdev = true;
			to_be_changed = true;
		}
	}

	for (i = 0; ; i++) {
		if (file_name[i]) {
            /* OSR
             *
             * Add parantheses to avoid compiler stylistic warning
             */
#ifdef HPE_mingw_BUILD
            
            if ((file_name[i] >= 0x01 && file_name[i] <= 0x1F) ||
                strchr(invalid_chars, file_name[i])) {
#else
            if (file_name[i] >= 0x01 && file_name[i] <= 0x1F ||
                strchr(invalid_chars, file_name[i])) {
#endif
				file_name[i] = '_';
				to_be_changed = true;
			}
		}
		else
			break;
	}

	return to_be_changed;
}

/**
 *  Generate target file name candidate from source file prefix,
 *  extension, and suffix.
 * @param file_name file_name to be checked
 */
char * _generate_target_file_name(const char *prefix, const char *extension, int suffix, bool dosdev)
{
	char *target;
	char suffix_string[LTFS_FILENAME_MAX*4+1];
	char trimmed_name[LTFS_FILENAME_MAX*4+1];
	int prefix_length, extension_length, suffix_length, ret;

	ret = -1;
	target = NULL;

	if (suffix) {
		sprintf( suffix_string, "~%d", suffix );

		prefix_length    = prefix    ? _utf8_strlen(prefix)    : 0;
		extension_length = extension ? _utf8_strlen(extension) : 0;
		suffix_length    = _utf8_strlen(suffix_string);

		if (prefix_length + extension_length + suffix_length > LTFS_FILENAME_MAX) {
			/* Need to trim source file name to add suffix */
			if (! dosdev && prefix_length > suffix_length) {
				/* Prefix is to be trimmed. */
				_utf8_strncpy(trimmed_name, prefix, prefix_length - suffix_length);
				if (extension)
					ret = asprintf(&target, "%s%s.%s", trimmed_name, suffix_string, extension);
				else
					ret = asprintf(&target, "%s%s", trimmed_name, suffix_string);

			} else if (extension_length > suffix_length) {
				/* Extension is to be trimmed. */
				_utf8_strncpy(trimmed_name, extension, extension_length - suffix_length);
				ret = asprintf(&target, "%s%s.%s", prefix, suffix_string, trimmed_name);
			} else {
				/* Unable to generate target file name. NULL is to be returned. */
			}
		} else {
			if (extension)
				ret = asprintf(&target, "%s%s.%s", prefix, suffix_string, extension);
			else
				ret = asprintf(&target, "%s%s", prefix, suffix_string);
		}
	} else {
		if (extension)
			ret = asprintf(&target, "%s.%s", prefix, extension);
		else {
			target = strdup(prefix);
			ret = target ? (int) strlen(target) : -1;
		}
	}

	return ret > 0 ? target : NULL;
}

/**
 *  Return the length of specified UTF-8 string.
 * @param s string to be counted.
 */
int _utf8_strlen(const char *s)
{
	int ret = 0;

	CHECK_ARG_NULL(s, -LTFS_NULL_ARG);

	while (*s) {
		if (! (*s & 0x80) || (*s & 0xC0) == 0xC0)
			++ret;
		++s;
	}
	return ret;
}

/**
 *  Copy UTF-8 string.
 * @param t Target buffer to store the string.
 * @param s Source string to be copied.
 * @param n Maximum length to be copied.
 */
int _utf8_strncpy(char *t, const char *s, int n)
{
	int ret = 0;

	CHECK_ARG_NULL(t, -LTFS_NULL_ARG);
	CHECK_ARG_NULL(s, -LTFS_NULL_ARG);

	while (*s) {
		if (! (*s & 0x80) || (*s & 0xC0) == 0xC0) {
			++ret;
			if (ret > n) {
				*t = 0x00;
				break;
			}
		}
		*t = *s;
		++t;
		++s;
	}
	return ret;
}

#endif
                                                              
//-Function------------------------------------------------------------------
// Name:        perform_name_percent_encoding
//
// Description: This function is called when a file is transfered to the LTFS
//              volume and checks to see if any of the characters need to be
//              percent encoded.  If needed then a percentencded flag is set
//              to true and the name that will appear in the index is set.
//
//---------------------------------------------------------------------------
// Parameters:  dentry  struct that holds info about the file/dir that has
//                      been transfered
//
//---------------------------------------------------------------------------
// Globals:     
//              
//
//---------------------------------------------------------------------------
// Returns:     updates the name field and percentencoded flag in dentry
//
//---------------------------------------------------------------------------
// Notes: HPE new function to support percent encoding for SNIA 2.4      
//
//---------------------------------------------------------------------------
// References:  SNIA 2.4.0 spec section 7.4
//
//--F------------------------------------------------------------------------

void perform_name_percent_encoding(struct dentry* dentry)
{ 
   char source_name[LTFS_FILENAME_MAX*4+1] = {0};  // To hold original file name
   char destination_name[LTFS_FILENAME_MAX*4+1] = {0}; // To hold return filename may or may not be percent encoded

   dentry->name = NULL;
   
   strcpy(source_name, dentry->platform_safe_name);
   strcpy(destination_name, source_name);

   dentry->percentencoded =  percent_encode_name(destination_name);
   
   if (dentry->percentencoded)
   {     
     
      ltfsmsg(LTFS_DEBUG, "17321D", source_name);
      ltfsmsg(LTFS_DEBUG, "17322D", destination_name);
      
   }
   
   dentry->name = strdup(destination_name);  // This may or may not be actually encoded
}

//-Function------------------------------------------------------------------
// Name:        perform_xattr_percent_encoding
//
// Description: This function is called when an attribute is set
//              and checks to see if any of the characters need to be
//              percent encoded.  If needed then a percentencded flag is set
//              to true and the name that will appear in the index is set.
//
//---------------------------------------------------------------------------
// Parameters:  xattr  struct that holds info about attribute
//
//---------------------------------------------------------------------------
// Globals:     
//              
//
//---------------------------------------------------------------------------
// Returns:     updates the percent_encoded_key field and percentencoded flag 
//              in xattr
//
//---------------------------------------------------------------------------
// Notes: HPE New function to support percent encoding for SNIA 2.4      
//
//---------------------------------------------------------------------------
// References:  SNIA 2.4.0 spec section 7.4
//
//--F------------------------------------------------------------------------
void perform_xattr_percent_encoding(struct xattr_info* xattr)
{ 
   char source_name[LTFS_FILENAME_MAX * 4 + 1] = {0};      // To hold original file name
   char destination_name[LTFS_FILENAME_MAX * 4 + 1] = {0}; // To hold return filename may or may not be percent encoded
   
   strcpy(source_name, xattr->key); // This is the unencoded key
   strcpy(destination_name, source_name);

   xattr->percentencoded =  percent_encode_name(destination_name);
   
   
   if (xattr->percentencoded) 
   {
      
      ltfsmsg(LTFS_DEBUG, "17321D", source_name);
      ltfsmsg(LTFS_DEBUG, "17322D", destination_name);
      
   }
   
   xattr->percent_encoded_key = strdup(destination_name);  // This is the name that will be written to the xml in the index.

}
