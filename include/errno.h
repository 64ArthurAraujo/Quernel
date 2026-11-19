#pragma once

#define ENOENT 2
#define EIO 5
#define ENOMEM 12
#define EEXIST 17
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define EFBIG 27
#define ENOSPC 28
#define ENAMETOOLONG 36
#define ESTALE 116

/**
 * errno_str():
 * 
 * Turns the errno's name based on its code.
 */
static inline const char* errno_str(int err) {
    switch (err) {
        case -ENOENT: case ENOENT:             return "ENOENT";
        case -EIO: case EIO:                   return "EIO";
        case -ENOMEM: case ENOMEM:             return "ENOMEM";
        case -EEXIST: case EEXIST:             return "EEXIST";
        case -ENOTDIR: case ENOTDIR:           return "ENOTDIR";
        case -EISDIR: case EISDIR:             return "EISDIR";
        case -EINVAL: case EINVAL:             return "EINVAL";
        case -EFBIG: case EFBIG:               return "EFBIG";
        case -ENOSPC: case ENOSPC:             return "ENOSPC";
        case -ENAMETOOLONG: case ENAMETOOLONG: return "ENAMETOOLONG";
        case -ESTALE: case ESTALE:             return "ESTALE";
        default:                               return "";
    }
}