#pragma once

#include "data/EnrolmentRepository.h"
#include "data/NotificationRepository.h"
#include "domain/Observer.h"
#include "domain/User.h"
#include "util/Clock.h"

namespace Lms {

class NotificationService : public IObserver {
public:
    NotificationService(IEnrolmentRepository& enrolments, INotificationRepository& notifications, const Clock& clock);
    void onEvent(const Event& event) override;
    std::vector<Notification> listUnread(const User& user) const;
    void markRead(const User& user, int notificationId);
private:
    IEnrolmentRepository& m_enrolments;
    INotificationRepository& m_notifications;
    const Clock& m_clock;
};

} // namespace Lms
