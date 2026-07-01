#include "mcs_vfs.h"

static size_t mcs_strlen(const char *s) {
    size_t n = 0;
    if (!s) {
        return 0;
    }
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

static int mcs_streq_n(const char *a, const char *b, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return b[n] == '\0';
}

static void mcs_copy_bytes(uint8_t *dst, const uint8_t *src, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

static void mcs_copy_name(char *dst, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < MCS_MAX_NAME; i++) {
        dst[i] = '\0';
    }
    for (i = 0; i < n && i + 1u < MCS_MAX_NAME; i++) {
        dst[i] = src[i];
    }
}

static int mcs_find_child(mcs_ramfs_t *fs, uint32_t parent, const char *name, size_t name_len, mcs_vnode_t **out) {
    size_t i;
    if (!fs || !name || !out || name_len == 0u || name_len >= MCS_MAX_NAME) {
        return MCS_EINVAL;
    }
    for (i = 0; i < fs->node_count; i++) {
        if (fs->nodes[i].used && fs->nodes[i].parent == parent && mcs_streq_n(name, fs->nodes[i].name, name_len)) {
            *out = &fs->nodes[i];
            return MCS_OK;
        }
    }
    return MCS_ENOENT;
}

static int mcs_split_parent_leaf(mcs_ramfs_t *fs, const char *path, mcs_vnode_t **parent, const char **leaf, size_t *leaf_len) {
    const char *seg;
    const char *next;
    mcs_vnode_t *cur;
    size_t seg_len;
    int rc;
    if (!fs || !path || !parent || !leaf || !leaf_len) {
        return MCS_EINVAL;
    }
    if (path[0] != '/') {
        return MCS_EINVAL;
    }
    if (path[1] == '\0') {
        return MCS_EINVAL;
    }
    if (mcs_strlen(path) >= MCS_MAX_PATH) {
        return MCS_ENAMETOOLONG;
    }
    cur = &fs->nodes[0];
    seg = path + 1;
    for (;;) {
        next = seg;
        while (*next != '/' && *next != '\0') {
            next++;
        }
        seg_len = (size_t)(next - seg);
        if (seg_len == 0u || seg_len >= MCS_MAX_NAME) {
            return MCS_EINVAL;
        }
        if (*next == '\0') {
            *parent = cur;
            *leaf = seg;
            *leaf_len = seg_len;
            return MCS_OK;
        }
        rc = mcs_find_child(fs, cur->id, seg, seg_len, &cur);
        if (rc != MCS_OK) {
            return rc;
        }
        if (cur->type != MCS_VNODE_DIR) {
            return MCS_ENOTDIR;
        }
        seg = next + 1;
    }
}

static int mcs_alloc_node(mcs_ramfs_t *fs, uint32_t parent, mcs_vnode_type_t type, const char *name, size_t name_len, size_t capacity, mcs_vnode_t **out) {
    mcs_vnode_t *node;
    if (!fs || !name || !out || name_len == 0u || name_len >= MCS_MAX_NAME) {
        return MCS_EINVAL;
    }
    if (fs->node_count >= MCS_MAX_NODES) {
        return MCS_ENOSPC;
    }
    if (type == MCS_VNODE_FILE && fs->data_used + capacity > MCS_RAMFS_DATA_BYTES) {
        return MCS_ENOSPC;
    }
    node = &fs->nodes[fs->node_count];
    node->used = 1u;
    node->id = (uint32_t)fs->node_count;
    node->parent = parent;
    node->type = type;
    mcs_copy_name(node->name, name, name_len);
    node->size = 0u;
    node->data_offset = 0u;
    node->data_capacity = 0u;
    if (type == MCS_VNODE_FILE) {
        node->data_offset = fs->data_used;
        node->data_capacity = capacity;
        fs->data_used += capacity;
    }
    fs->node_count++;
    *out = node;
    return MCS_OK;
}

void mcs_ramfs_init(mcs_ramfs_t *fs) {
    size_t i;
    if (!fs) {
        return;
    }
    for (i = 0; i < MCS_MAX_NODES; i++) {
        fs->nodes[i].used = 0u;
        fs->nodes[i].id = 0u;
        fs->nodes[i].parent = 0u;
        fs->nodes[i].type = MCS_VNODE_FILE;
        fs->nodes[i].name[0] = '\0';
        fs->nodes[i].size = 0u;
        fs->nodes[i].data_offset = 0u;
        fs->nodes[i].data_capacity = 0u;
    }
    for (i = 0; i < MCS_RAMFS_DATA_BYTES; i++) {
        fs->data[i] = 0u;
    }
    fs->node_count = 1u;
    fs->data_used = 0u;
    fs->nodes[0].used = 1u;
    fs->nodes[0].id = 0u;
    fs->nodes[0].parent = 0u;
    fs->nodes[0].type = MCS_VNODE_DIR;
    fs->nodes[0].name[0] = '/';
    fs->nodes[0].name[1] = '\0';
}

int mcs_ramfs_lookup(mcs_ramfs_t *fs, const char *path, mcs_vnode_t **out_node) {
    const char *seg;
    const char *next;
    mcs_vnode_t *cur;
    size_t seg_len;
    int rc;
    if (!fs || !path || !out_node) {
        return MCS_EINVAL;
    }
    if (path[0] != '/') {
        return MCS_EINVAL;
    }
    if (mcs_strlen(path) >= MCS_MAX_PATH) {
        return MCS_ENAMETOOLONG;
    }
    if (path[1] == '\0') {
        *out_node = &fs->nodes[0];
        return MCS_OK;
    }
    cur = &fs->nodes[0];
    seg = path + 1;
    while (*seg != '\0') {
        next = seg;
        while (*next != '/' && *next != '\0') {
            next++;
        }
        seg_len = (size_t)(next - seg);
        if (seg_len == 0u || seg_len >= MCS_MAX_NAME) {
            return MCS_EINVAL;
        }
        rc = mcs_find_child(fs, cur->id, seg, seg_len, &cur);
        if (rc != MCS_OK) {
            return rc;
        }
        if (*next == '\0') {
            *out_node = cur;
            return MCS_OK;
        }
        if (cur->type != MCS_VNODE_DIR) {
            return MCS_ENOTDIR;
        }
        seg = next + 1;
    }
    return MCS_EINVAL;
}

int mcs_ramfs_create_file(mcs_ramfs_t *fs, const char *path, mcs_vnode_t **out_node) {
    mcs_vnode_t *parent;
    mcs_vnode_t *existing;
    const char *leaf;
    size_t leaf_len;
    int rc;
    if (!fs || !path || !out_node) {
        return MCS_EINVAL;
    }
    rc = mcs_ramfs_lookup(fs, path, &existing);
    if (rc == MCS_OK) {
        if (existing->type != MCS_VNODE_FILE) {
            return MCS_EISDIR;
        }
        *out_node = existing;
        return MCS_OK;
    }
    rc = mcs_split_parent_leaf(fs, path, &parent, &leaf, &leaf_len);
    if (rc != MCS_OK) {
        return rc;
    }
    if (parent->type != MCS_VNODE_DIR) {
        return MCS_ENOTDIR;
    }
    return mcs_alloc_node(fs, parent->id, MCS_VNODE_FILE, leaf, leaf_len, 256u, out_node);
}

int mcs_ramfs_seed_file(mcs_ramfs_t *fs, const char *path, const uint8_t *data, size_t len) {
    mcs_vnode_t *node;
    int rc;
    if (!fs || !path || (!data && len != 0u)) {
        return MCS_EINVAL;
    }
    rc = mcs_ramfs_create_file(fs, path, &node);
    if (rc != MCS_OK) {
        return rc;
    }
    if (len > node->data_capacity) {
        return MCS_ENOSPC;
    }
    mcs_copy_bytes(&fs->data[node->data_offset], data, len);
    node->size = len;
    return MCS_OK;
}
