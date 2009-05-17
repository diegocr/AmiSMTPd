/* ***** BEGIN LICENSE BLOCK *****
 * Version: MIT/X11 License
 * 
 * Copyright (c) 2004 Diego Casorran
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * Contributor(s):
 *   Diego Casorran <dcasorran@gmail.com> (Original Author)
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef SMTPD_DEBUG_HOSTADDR_H
#define SMTPD_DEBUG_HOSTADDR_H

  #ifndef NDEBUG
  kprintf("Host %s :\n", smtpserv);
  kprintf("  Officially:\t%s\n", hostaddr->h_name);
  kprintf("  Aliases:\t");

  for(i=0; hostaddr->h_aliases[i]; ++i)
  {
    if(i) kprintf(", ");
    kprintf("%s", hostaddr->h_aliases[i]);
  }

  kprintf("\n  Type:\t\t%s\n", hostaddr->h_addrtype == AF_INET ? "AF_INET" : "AF_INET6");
  if(hostaddr->h_addrtype == AF_INET)
  {
    for(i=0; hostaddr->h_addr_list[i]; ++i)
    {
      kprintf("  Address:\t%s\n", Inet_NtoA(((struct in_addr *)hostaddr->h_addr_list[i])->s_addr));
    }
  }
  #endif


#endif /* SMTPD_DEBUG_HOSTADDR_H */
