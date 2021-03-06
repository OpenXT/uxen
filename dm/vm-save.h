
#ifndef _VM_SAVE_H_
#define _VM_SAVE_H_

#ifdef _WIN32
#define SAVE_CUCKOO_ENABLED
#endif

enum vm_save_compress_mode {
    VM_SAVE_COMPRESS_NONE = 1,
    VM_SAVE_COMPRESS_LZ4,
#ifdef SAVE_CUCKOO_ENABLED
    VM_SAVE_COMPRESS_CUCKOO,
    VM_SAVE_COMPRESS_CUCKOO_SIMPLE,
#endif
};

#define _m(v) (1 << (v))
#define vm_save_compress_mode_batched(m)                                \
    (!!(_m(m) & (_m(VM_SAVE_COMPRESS_NONE) | _m(VM_SAVE_COMPRESS_LZ4))))

struct vm_save_info {
    int awaiting_suspend;
    int save_requested;
    int save_abort;
    int save_via_temp;
    int safe_to_abort;
    ioh_event save_abort_event;

    char *filename;
    struct filebuf *f;
    off_t dm_offset;

    struct control_desc *command_cd;
    char *command_id;
    struct control_desc *resume_cd;
    char *resume_id;

    enum vm_save_compress_mode compress_mode;
    int single_page;
    int free_mem;
    int high_compress;
    int ignore_framebuffer;
    int fingerprint;

    int resume_delete;

    off_t page_batch_offset;
};

extern struct vm_save_info vm_save_info;

void vm_save(void);
void vm_save_set_abortable(void);
void vm_save_abort(void);
struct xc_dominfo;
int vm_process_suspend(struct xc_dominfo *info);
void vm_save_execute(void);
void vm_save_finalize(void);
char *vm_save_file_name(const unsigned char *uuid);
int vm_save_read_dm_offset(void *dst, off_t offset, size_t size);

int vm_resume(void);

int vm_load(const char *, int);
int vm_load_finish(void);

int vm_lazy_load_page(uint32_t gpfn, uint8_t *va, int compressed);

#ifdef SAVE_CUCKOO_ENABLED
struct page_fingerprint;

int
save_cuckoo_pages(struct filebuf *f, struct page_fingerprint *hashes,
                  int n, int simple_mode, char **err_msg);
#endif

#endif	/* _VM_SAVE_H_ */
