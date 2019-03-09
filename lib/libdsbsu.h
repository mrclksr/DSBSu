/*-
 * Copyright (c) 2018 Marcel Kaiser. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _DSBSU_H_
#define _DSBSU_H_
#include <sys/cdefs.h>

#define DSBSU_ERR_FATAL	  (1 << 0)
#define DSBSU_EAUTH	  (1 << 1)
#define DSBSU_EEXECCMD	  (1 << 2)
#define DSBSU_ENOUSER	  (1 << 3)
#define DSBSU_EUNEXPECTED (1 << 4)
#define DSBSU_ETIMEOUT	  (1 << 5)
#define DSBSU_EEXECSU	  (1 << 7)
#define DSBSU_ERR_SYS	  (1 << 8)

typedef struct dsbsu_proc_s dsbsu_proc;

__BEGIN_DECLS
extern int	  dsbsu_error(void);
extern int	  dsbsu_wait(dsbsu_proc *);
extern bool	  dsbsu_validate_user(const char *);
extern bool	  dsbsu_is_me(const char *);
extern dsbsu_proc *dsbsu_exec_su(const char *, const char *, const char *);
extern const char *dsbsu_strerror(void);
__END_DECLS

#endif	/* !_DSBSU_H_ */
