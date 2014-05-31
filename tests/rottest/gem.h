#ifndef __GEM_H__
#define __GEM_H__

extern int exynos_gem_create(int fd, struct drm_exynos_gem_create *gem);
extern int exynos_gem_userptr(int fd,
				struct drm_exynos_gem_userptr *gem_userptr);
extern int exynos_gem_mmap(int fd, struct drm_exynos_gem_mmap *in_mmap);
extern int exynos_gem_close(int fd, struct drm_gem_close *gem_close);
extern int exynos_gem_cache_op(int fd,
				struct drm_exynos_gem_cache_op *cache_op);

#endif
