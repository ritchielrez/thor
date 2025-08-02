#ifndef BUILD_H_INCLUDED
#define BUILD_H_INCLUDED

#include <assert.h>
#include <stdbool.h>

#include "defines.h"
#define RIT_STR_IMPLEMENTATION
#include "external/rit_dyn_arr.h"
#include "external/rit_str.h"

#if defined(BUILD_WINDOWS)
typedef HANDLE cmd_proc_t;
#define CMD_INVALID_PROC INVALID_HANDLE_VALUE
#else
typedef int cmd_proc_t;
#define CMD_INVALID_PROC (-1)
#endif  // BUILD_WINDOWS

typedef rda_struct(char *) cmd_t;
typedef rda_allocator cmd_allocator;

static inline void *ctx_allocator_alloc(void *t_ctx, size_t t_size_in_bytes) {
  (void)t_ctx;
  return malloc(t_size_in_bytes);
}

static inline void ctx_allocator_free(void *t_ctx, void *t_ptr) {
  (void)t_ctx;
  free(t_ptr);
}

static inline void *ctx_allocator_realloc(void *t_ctx, void *t_old_ptr,
                            size_t t_old_size_in_bytes,
                            size_t t_new_size_in_bytes) {
  (void)t_ctx;
  (void)t_old_size_in_bytes;
  return realloc(t_old_ptr, t_new_size_in_bytes);
}

#define cmd(t_cmd, t_allocator) \
  cmd_t t_cmd = {};             \
  rda_init(t_cmd, 0, sizeof(char *), t_allocator)

#define cmd_init(t_cmd, t_size, t_allocator) \
  rda_init(t_cmd, t_size, sizeof(char *), t_allocator)

#define cmd_free rda_free

#define cmd_push_back rda_push_back
#define cmd_append_cmd rda_append_rda
#define cmd_append(t_cmd, t_allocator, ...) \
  rda_append_arr(t_cmd, ((char *[]){__VA_ARGS__}), t_allocator)

static inline struct rstr cmd_rstr(cmd_t *t_cmd, rstr_allocator *t_allocator) {
  rstr(result, rsv_lit(""), t_allocator);
  rda_for_each(it, (*t_cmd)) {
    rsv cmd_tok = rsv_lit(*it);
    rstr_append_str(result, cmd_tok, t_allocator);
    rstr_push_back(result, ' ', t_allocator);
  }
  return result;
}

#define cmd_run_async(t_cmd) cmd_run_async__(&t_cmd)

static inline cmd_proc_t cmd_run_async__(cmd_t *t_cmd) {
  // Code stolen from Alexey Kutepov's nob.h
  // (https://github.com/tsoding/musializer/blob/master/nob.h)
  rstr_allocator allocator = {ctx_allocator_alloc, ctx_allocator_free,
                              ctx_allocator_realloc, NULL};
#if defined(BUILD_WINDOWS)
  struct rstr cmd = cmd_rstr(t_cmd, &allocator);
  STARTUPINFO startup_info;
  ZeroMemory(&startup_info, sizeof(startup_info));
  startup_info.cb = sizeof(STARTUPINFO);
  startup_info.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  startup_info.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  startup_info.dwFlags |= STARTF_USESTDHANDLES;

  PROCESS_INFORMATION proc_info;
  ZeroMemory(&proc_info, sizeof(PROCESS_INFORMATION));

  // TODO: use a more reliable rendering of the command instead of
  BOOL success = CreateProcessA(NULL, rstr_cstr(cmd), NULL, NULL, TRUE, 0, NULL,
                                NULL, &startup_info, &proc_info);
  cmd_free(cmd, &allocator);

  if (!success) {
    fprintf(stderr, "Could not create child process: %lu\n", GetLastError());
    return CMD_INVALID_PROC;
  }

  CloseHandle(proc_info.hThread);
  return proc_info.hProcess;
#else
  // NOTE: There is an intentional memory leak here, it is not possible to free
  // `cmd` after `execvp()` is called.
  rda_cp(char *, cmd, *t_cmd, 0, rda_size(*t_cmd), &allocator);
  rda_push_back(cmd, NULL, &allocator);
  pid_t cpid = fork();
  if (cpid < 0) {
    fprintf(stderr, "Could not create child process: %s\n", strerror(errno));
    return CMD_INVALID_PROC;
  }
  if (cpid == 0) {
    // Inside the child process
    if (execvp(rda_at(*t_cmd, 0), (char *const *)rda_data(cmd)) < 0) {
      fprintf(stderr, "Could not exe child process: %s\n", strerror(errno));
      exit(1);
    }
    assert(0 && "unreachable");
  }
  return cpid;
#endif  // BUILD_WINDOWS
}

static inline bool cmd_proc_wait(cmd_proc_t t_proc) {
  if (t_proc == CMD_INVALID_PROC) return false;
    // Code stolen from Alexey Kutepov's nob.h
    // (https://github.com/tsoding/musializer/blob/master/nob.h)
#if defined(BUILD_WINDOWS)
  DWORD result = WaitForSingleObject(t_proc,   // HANDLE hHandle,
                                     INFINITE  // DWORD  dwMilliseconds
  );

  if (result == WAIT_FAILED) {
    fprintf(stderr, "Could not wait on child process: %lu\n", GetLastError());
    return false;
  }

  DWORD exit_status;
  if (!GetExitCodeProcess(t_proc, &exit_status)) {
    fprintf(stderr, "Could not get process exit code: %lu\n", GetLastError());
    return false;
  }

  if (exit_status != 0) {
    fprintf(stderr, "Command exited with exit code: %lu\n", exit_status);
    return false;
  }

  CloseHandle(t_proc);
  return true;
#else
  while (true) {
    int wstatus = 0;
    if (waitpid(t_proc, &wstatus, 0) < 0) {
      fprintf(stderr, "Count not wait on command (pid %d): %s\n", t_proc,
              strerror(errno));
      return false;
    }
    if (WIFEXITED(wstatus)) {
      int exit_status = WEXITSTATUS(wstatus);
      if (exit_status != 0) {
        fprintf(stderr, "Command exited with exit code: %d\n", exit_status);
        return 1;
      }
      break;
    }
    if (WIFSIGNALED(wstatus)) {
      fprintf(stderr, "Command process was terminated by %s\n",
              strsignal(WTERMSIG(wstatus)));
      return false;
    }
  }
  return true;
#endif  // BUILD_WINDOWS
}

#define cmd_run_sync(t_cmd) cmd_run_sync__(&t_cmd)

static inline bool cmd_run_sync__(cmd_t *t_cmd) {
  cmd_proc_t proc = cmd_run_async__(t_cmd);
  return cmd_proc_wait(proc);
}

#endif  // BUILD_H_INCLUDED
