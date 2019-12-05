#include "handler.h"
#include "rootfs-handler.h"

int rootfsv4_handler_wrapper(struct img_type *img, void *data)
{
    return rootfsv4_handler(img, data);
}

__attribute__((constructor))
void rootfs_handler_init(void)
{
    register_handler("ROOTFSv4"
                     , rootfsv4_handler_wrapper
                     , IMAGE_HANDLER
                     , NULL);
}
