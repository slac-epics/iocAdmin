// --------------------------------------------------------
// $Id: HTMLPage.cpp,v 1.1.1.1 2006/07/13 14:11:13 luchini Exp $
//
// Please refer to NOTICE.txt,
// included as part of this distribution,
// for legal information.
//
// Kay-Uwe Kasemir, kasemir@lanl.gov
// --------------------------------------------------------

// System
#include <stdio.h>
#include <stdarg.h>
// Tools
#include <ToolsConfig.h>
#include <epicsTimeHelper.h>
#include <NetTools.h>
// Engine
#include "HTMLPage.h"

bool HTMLPage::with_config = true;

HTMLPage::HTMLPage(SOCKET socket, const char *title, int refresh)
  : socket(socket),
    refresh(refresh)
{
    line("HTTP/1.0 200 OK");
    line("Server: ArchiveEngine");
    line("Content-type: text/html");
    line("");

    line("<HTML>");
    if (refresh > 0)
    {
        char linebuf[60];
        sprintf(linebuf, "<META HTTP-EQUIV=\"Refresh\" CONTENT=%d>",
                refresh);
        line(linebuf);
    }

    line("");
    line("<HEAD>");
    out ("<TITLE>");
    out(title);
    line("</TITLE>");
    line("</HEAD>");
    line("");
    line("<BODY BGCOLOR=#A7ADC6 LINK=#0000FF VLINK=#0000FF ALINK=#0000FF>");
    line("<FONT FACE=\"Helvetica, Arial\">");
    line("<BLOCKQUOTE>");
    line("<H1>");
    line(title);
    line("</H1>");
    line("");
    line("<P>");
    line("");
}

HTMLPage::~HTMLPage()
{
    line("<P><HR WIDTH=50% ALIGN=LEFT>");

    line("<A HREF=\"/\">-Main-</A>  ");
    line("<A HREF=\"/groups\">-Groups-</A>  ");
    if (with_config)
        line("<A HREF=\"/config\">-Config.-</A>  ");
    line("<BR>");
    
    char linebuf[100];
    if (refresh > 0)
    {
        sprintf(linebuf, "This page will update every %d seconds...",
                refresh);
        line(linebuf);
    }
    else
    {
        int year, month, day, hour, min, sec;
        unsigned long nano;
        epicsTime2vals(epicsTime::getCurrent(),
                       year, month, day, hour, min, sec, nano);
        sprintf(linebuf,
                "(Status for %02d/%02d/%04d %02d:%02d:%02d. "
                "Use <I>Reload</I> from the Browser's menu for updates)",
                month, day, year, hour, min, sec);
        line(linebuf);
    }
    line("</BLOCKQUOTE>");
    line("</FONT>");
    line("</BODY>");
    line("</HTML>");
    // Mozilla would quite often not display the full page.
    // Did these empty lines at the end of the document fix that?!
    out("\r\n", 2);
    out("\r\n", 2);
    out("\r\n", 2);
}

void HTMLPage::out(const char *line, size_t length)
{    
    // Used to use send, but man page claims send with 0 flags
    // is the same as write, and write has the advantage of
    // also working with stdout, so one can create unit tests
    // w/o any sockets.
    // ::send(socket, line, length, 0);
    ::write(socket, line, length);
}

void HTMLPage::out(const char *line)
{    out(line, strlen(line));    }

void HTMLPage::out(const stdString &line)
{    out(line.c_str(), line.length ());    }


void HTMLPage::line(const char *line)
{
    out(line);
    out("\r\n", 2);
}

void HTMLPage::line(const stdString &line)
{
    out(line);
    out("\r\n", 2);
}

// Last column name must be 0
// Used to use size_t, but the default type in calls
//  openTable(1, "xx", 2, "yy", 0)
// must be int, and when expecting size_t in va-arg,
// valgrind warned about uninitialized memory on 64bit OSs.
void HTMLPage::openTable(int colspan, const char *column, ...)
{
    va_list    ap;
    const char *name = column;

    va_start(ap, column);
    line("<TABLE BORDER=0 CELLPADDING=5>");
    out("<TR>");
    while (name)
    {
        if (colspan > 1)
        {
            char buf[80];
            sprintf(buf, "<TH COLSPAN=%d BGCOLOR=#000000><FONT COLOR=#FFFFFF>",
                    (int)colspan);
            out(buf);
        }
        else
            out("<TH BGCOLOR=#000000><FONT COLOR=#FFFFFF>");
        out(name);
        out("</FONT></TH>");

        colspan = va_arg(ap, int);
        if (!colspan)
            break;
        name = va_arg(ap, const char *);
    }
    line("</TR>");
    va_end(ap);
}

void HTMLPage::tableLine(const char *item, ...)
{
    va_list    ap;
    const char *name = item;
    bool first = true;

    va_start(ap, item);
    out("<TR>");
    while (name)
    {
        if (first)
            out("<TH BGCOLOR=#FFFFFF>");
        else
            out("<TD ALIGN=CENTER>");
        out(name);
        if (first)
        {
            out("</TH>");
            first = false;
        }
        else
            out("</TD>");
        name = va_arg(ap, const char *);
    }

    line("</TR>");
    va_end(ap);
}

void HTMLPage::closeTable()
{
    line("</TABLE>");
}

