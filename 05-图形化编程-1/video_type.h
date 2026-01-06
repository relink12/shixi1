#ifndef VIDEO_TYPE_H
#define VIDEO_TYPE_H

// 统一声明视频文件结构体，避免重复定义
typedef struct {
    const char *name;  // 视频显示名称
    const char *path;  // 视频文件路径
} VideoFile;

#endif