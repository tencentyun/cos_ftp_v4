/*
 * Part of Very Secure FTPd
 * Licence: GPL v2
 * Author: Chris Evans
 * access.c
 *
 * Routines to do very very simple access control based on filenames.
 */

#include "access.h"
#include "ls.h"
#include "tunables.h"
#include "str.h"
#include "sysstr.h"
#include <stdlib.h>
#include <string.h>

extern void get_user_root_dir(struct mystr* p_root_dir, struct vsf_session* p_sess);
extern void session_real_path(struct vsf_session* p_sess, struct mystr* p_path);

int
vsf_access_check_file(const struct mystr* p_filename_str)
{
  static struct mystr s_access_str;
  unsigned int iters = 0;

  if (!tunable_deny_file)
  {
    return 1;
  }
  if (str_isempty(&s_access_str))
  {
    str_alloc_text(&s_access_str, tunable_deny_file);
  }
  if (vsf_filename_passes_filter(p_filename_str, &s_access_str, &iters))
  {
    return 0;
  }
  else
  {
    struct str_locate_result loc_res =
      str_locate_str(p_filename_str, &s_access_str);
    if (loc_res.found)
    {
      return 0;
    }
  }
  return 1;
}

#if 1 //modify 20150919
int
vsf_access_session_check_file(struct vsf_session* p_sess, const struct mystr* p_filename_str)
{
	static struct mystr s_real_path, s_user_root;
	str_copy(&s_real_path, p_filename_str);

	session_real_path(p_sess, &s_real_path);
	if (str_isempty(&s_real_path) || str_get_char_at(&s_real_path, str_getlen(&s_real_path)-1) != '/')
		str_append_char(&s_real_path, '/');

	get_user_root_dir(&s_user_root, p_sess);
	str_append_char(&s_user_root, '/');
	if (strncmp(str_getbuf(&s_user_root), str_getbuf(&s_real_path), str_getlen(&s_user_root)) != 0)
		return 0;

	return vsf_access_check_file(p_filename_str);
}
#endif

int
vsf_access_check_file_visible(const struct mystr* p_filename_str)
{
  static struct mystr s_access_str;
  unsigned int iters = 0;

  if (!tunable_hide_file)
  {
    return 1;
  }
  if (str_isempty(&s_access_str))
  {
    str_alloc_text(&s_access_str, tunable_hide_file);
  }
  if (vsf_filename_passes_filter(p_filename_str, &s_access_str, &iters))
  {
    return 0;
  }
  else
  {
    struct str_locate_result loc_res =
      str_locate_str(p_filename_str, &s_access_str);
    if (loc_res.found)
    {
      return 0;
    }
  }
  return 1;
}

