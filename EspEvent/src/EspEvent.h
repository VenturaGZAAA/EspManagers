//
// Created by urzu-7 on 11/10/26.
//

#ifndef ESPMANAGERS_ESPEVENT_H
#define ESPMANAGERS_ESPEVENT_H
#include <functional>
#include <vector>
#include <mutex>
#include <algorithm>
#include <optional>

template<typename... Args>
class EspEvent {
    using Handler = std::function<void(Args...)>;
    struct Entry {
        Handler handler;
        uint32_t id;
    };
    std::vector<Entry> handlers;
    uint32_t nextId = 0;
    mutable std::mutex mtx;

public:
    // Returns a unique ID used to later unsubscribe
    uint32_t operator+=(Handler handler) {
        std::lock_guard lock(mtx);
        uint32_t id = nextId++;
        handlers.push_back({std::move(handler), id});
        return id;
    }

    void operator-=(uint32_t id) {
        std::lock_guard lock(mtx);
        handlers.erase(
            std::remove_if(handlers.begin(), handlers.end(),
                [id](const Entry& e) { return e.id == id; }),
            handlers.end()
        );
    }

    void operator()(Args... args) const {
        std::lock_guard lock(mtx);
        for (const auto& entry : handlers) {
            entry.handler(args...);
        }
    }
    uint8_t subs() {
        std::lock_guard lock(mtx);
        return handlers.size();
    }
};
#endif //ESPMANAGERS_ESPEVENT_H
