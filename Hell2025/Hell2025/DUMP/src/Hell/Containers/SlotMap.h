#pragma once
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Hell {

    template<class T>
    class SlotMap {
    public:
        using value_type = T;

        SlotMap() = default;
        explicit SlotMap(size_t reserveCount) { reserve(reserveCount); }

        void reserve(size_t n) {
            m_values.reserve(n);
            m_denseToSlot.reserve(n);
            m_denseToId.reserve(n);
            m_idToSlot.reserve(n);
        }

        size_t size()  const { return m_values.size(); }
        bool   empty() const { return m_values.empty(); }

        void clear() {
            m_values.clear();
            m_denseToSlot.clear();
            m_denseToId.clear();
            m_idToSlot.clear();
            m_slotToDense.assign(m_slotToDense.size(), kInvalid);
            m_free.clear();
        }

        template<class... Args>
        bool emplace_with_id(uint64_t id, Args&&... args) {
            if (m_idToSlot.find(id) != m_idToSlot.end()) return false;
            const uint32_t slot = alloc_slot();
            const uint32_t di = (uint32_t)m_values.size();

            m_values.emplace_back(std::forward<Args>(args)...);
            m_denseToSlot.push_back(slot);
            m_slotToDense[slot] = di;

            m_denseToId.push_back(id);
            m_idToSlot.emplace(id, slot);
            return true;
        }

        T* get(uint64_t id) {
            auto it = m_idToSlot.find(id);
            if (it == m_idToSlot.end()) return nullptr;
            const uint32_t slot = it->second;
            if (slot >= m_slotToDense.size()) return nullptr;
            const uint32_t di = m_slotToDense[slot];
            if (di == kInvalid) return nullptr;
            return &m_values[di];
        }
        const T* get(uint64_t id) const { return const_cast<SlotMap*>(this)->get(id); }

        bool contains(uint64_t id) const { return get(id) != nullptr; }

        bool erase(uint64_t id) {
            auto it = m_idToSlot.find(id);
            if (it == m_idToSlot.end()) return false;

            const uint32_t slot = it->second;
            if (slot >= m_slotToDense.size()) { m_idToSlot.erase(it); return false; }
            const uint32_t di = m_slotToDense[slot];
            if (di == kInvalid) { m_idToSlot.erase(it); return false; }

            const uint32_t back = (uint32_t)m_values.size() - 1;

            if (di != back) {
                std::swap(m_values[di], m_values[back]);

                const uint32_t movedSlot = m_denseToSlot[back];
                m_denseToSlot[di] = movedSlot;
                m_slotToDense[movedSlot] = di;

                std::swap(m_denseToId[di], m_denseToId[back]);
                const uint64_t movedId = m_denseToId[di];
                m_idToSlot[movedId] = movedSlot;
            }

            m_values.pop_back();
            m_denseToSlot.pop_back();

            const uint64_t backId = m_denseToId.back();
            m_denseToId.pop_back();
            m_idToSlot.erase(backId);

            m_slotToDense[slot] = kInvalid;
            m_free.push_back(slot);
            return true;
        }

        auto begin() { return m_values.begin(); }
        auto end() { return m_values.end(); }
        auto begin() const { return m_values.begin(); }
        auto end()   const { return m_values.end(); }

        T& operator[](size_t denseIndex) { return m_values[denseIndex]; }
        const T& operator[](size_t denseIndex) const { return m_values[denseIndex]; }

        T* data() { return m_values.data(); }
        const T* data() const { return m_values.data(); }

        uint64_t id_at(size_t denseIndex) const { return m_denseToId[denseIndex]; }

        uint32_t dense_index_of(uint64_t id) const {
            auto it = m_idToSlot.find(id);
            if (it == m_idToSlot.end()) return kInvalid;
            const uint32_t slot = it->second;
            if (slot >= m_slotToDense.size()) return kInvalid;
            return m_slotToDense[slot];
        }

        const std::vector<uint64_t>& ids() const { return m_denseToId; }

    private:
        static constexpr uint32_t kInvalid = 0xFFFFFFFFu;

        std::vector<T>        m_values;
        std::vector<uint32_t> m_denseToSlot;
        std::vector<uint32_t> m_slotToDense;
        std::vector<uint32_t> m_free;
        std::vector<uint64_t> m_denseToId;
        std::unordered_map<uint64_t, uint32_t> m_idToSlot;

        uint32_t alloc_slot() {
            if (!m_free.empty()) {
                const uint32_t s = m_free.back();
                m_free.pop_back();
                return s;
            }
            const uint32_t s = (uint32_t)m_slotToDense.size();
            m_slotToDense.push_back(kInvalid);
            return s;
        }
    };

}
