#ifndef KFX_MACOS_SAVEFILE_H
#define KFX_MACOS_SAVEFILE_H
#ifdef __cplusplus
extern "C" {
#endif
int kfx_save_is_protected(const char *destination);
int kfx_commit_save(const char *temporary, const char *destination);
#ifdef __cplusplus
}
#endif
#endif
