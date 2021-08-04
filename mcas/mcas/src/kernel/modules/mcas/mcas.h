enum {
  IOCTL_CMD_EXPOSE = 8,
  IOCTL_CMD_REMOVE = 9,
  IOCTL_CMD_QUERY  = 10,
};

typedef struct
{
  uint64_t token; /* token that must be used with the mmap call */
  void*    vaddr; /* address of memory to share (from calling process perspective) */
  size_t   vaddr_size; /* size of region to share */
}
 __attribute__((packed)) IOCTL_EXPOSE_msg;

typedef struct
{
  uint64_t token; /* token of previously exposed memory */
}
__attribute__((packed)) IOCTL_REMOVE_msg;  

typedef struct
{
  union {
    uint64_t token; /* token of previously exposed memory */
    size_t size;
  };
}
__attribute__((packed)) IOCTL_QUERY_msg;  




