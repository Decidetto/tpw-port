// itemlist_dispatch.cpp: the virtual calls CTPItemListResource's code makes through __ptr_glue
// (include/TPResource.h, namespace itemlist_callee), which unit v5375 declared as free
// functions so its tests could fake them. In the launcher they are the calls themselves: the
// file storage's OpenFile (vtable slot 0x14), and the deleting destructors of an item and of
// the item list (their hidden signed 16 bit flag: positive frees the object).
#include <cstdint>

#include "TPResource.h"
#include "bfl_FileSystem.h"

namespace itemlist_callee {

int32_t StorageOpen(bfl::FileSystem::CFileStorage* storage, bfl::FileSystem::CFile& file, const char* name,
                    uint32_t mode, void* data) {
    if (storage == nullptr) return -1;
    TbError result = storage->OpenFile(file, name, mode, data);
    return static_cast<int32_t>(result);
}

void ItemDelete(CTPItemResource* item, int16_t flag) {
    if (item == nullptr) return;
    if (flag > 0) delete item;
    else item->~CTPItemResource();
}

void ItemListDelete(TbPtrList<CTPItemResource>* list, int16_t flag) {
    if (list == nullptr) return;
    if (flag > 0) delete list;
    else list->~TbPtrList<CTPItemResource>();
}

}  // namespace itemlist_callee
