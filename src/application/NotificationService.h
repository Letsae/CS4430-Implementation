#pragma once

#include "data/EnrolmentRepository.h"
#include "data/NotificationRepository.h"
#include "domain/Observer.h"
#include "util/Clock.h"

namespace Lms {

class NotificationService : public IObserver {
public:
    NotificationService(IEnrolmentRepository& enrolments, INotificationRepository& notifications, const Clock& clock);
    void onEvent(const Event& event) override;
private:
    IEnrolmentRepository& m_enrolments;
    INotificationRepository& m_notifications;
    const Clock& m_clock;
};

} // namespace Lms
