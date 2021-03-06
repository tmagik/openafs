/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* $Id$ */

#ifndef __KHIMAIRA_KHHTLINK_H
#define __KHIMAIRA_KHHTLINK_H

/*! \addtogroup khui
@{ */

/*! \defgroup khui_hyperlink Hyperlink
@{*/

/*! \brief A hyperlink

    When a link in a hypertext window is clicked, this structure is
    passed along with the message.

    The link text fields do to point to NULL terminated strings.
    Instead, the length fields should be used to extract the string.
 */
typedef struct tag_khui_htwnd_link {
    RECT r;
    wchar_t * id;
    int id_len;
    wchar_t * param;
    int param_len;
} khui_htwnd_link;

#define KHUI_MAXCCH_HTLINK_FIELD 256
#define KHUI_MAXCB_HTLINK_FIELD (KHUI_MAXCCH_HTLINK_FIELD * sizeof(wchar_t))

/*!@}*/
/*!@}*/

#endif
