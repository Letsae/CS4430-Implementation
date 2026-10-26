#include "Observer.h"
#include <algorithm>

namespace Lms {

void EventPublisher::subscribe(std::shared_ptr<IObserver> observer) {
    m_observers.push_back(observer);
}

void EventPublisher::unsubscribe(std::shared_ptr<IObserver> observer) {
    m_observers.erase(
        std::remove_if(m_observers.begin(), m_observers.end(),
            [&observer](const std::weak_ptr<IObserver>& weak) {
                return weak.lock() == observer;
            }),
        m_observers.end()
    );
}

void EventPublisher::publish(const Event& event) {
    // Remove expired observers
    m_observers.erase(
        std::remove_if(m_observers.begin(), m_observers.end(),
            [](const std::weak_ptr<IObserver>& weak) {
                return weak.expired();
            }),
        m_observers.end()
    );

    // Notify remaining observers
    for (auto& weakObserver : m_observers) {
        if (auto observer = weakObserver.lock()) {
            observer->onEvent(event);
        }
    }
}

} // namespace Lms
