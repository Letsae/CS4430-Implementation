#include "NotificationService.h"
#include "domain/LmsException.h"

namespace Lms {

NotificationService::NotificationService(IEnrolmentRepository& enrolments,
        INotificationRepository& notifications, const Clock& clock)
    : m_enrolments(enrolments), m_notifications(notifications), m_clock(clock) {}

void NotificationService::onEvent(const Event& event) {
    if (const auto* late = dynamic_cast<const LateSubmissionEvent*>(&event)) {
        Notification notification(0, late->getStudentId(),
            "Your late submission was received for assignment " + std::to_string(late->getAssignmentId()),
            m_clock.now(), false);
        m_notifications.save(notification);
        return;
    }
    if (const auto* assigned = dynamic_cast<const LecturerAssignedEvent*>(&event)) {
        const std::string message = "You have been assigned to course " + assigned->getCourseCode();
        Notification notification(0, assigned->getLecturerId(), message, m_clock.now(), false);
        m_notifications.save(notification);
        return;
    }
    if (const auto* released = dynamic_cast<const MarksReleasedEvent*>(&event)) {
        for (const int studentId : released->getStudentIds()) {
            Notification notification(0, studentId, "Marks have been released", m_clock.now(), false);
            m_notifications.save(notification);
        }
        return;
    }
    if (const auto* extension = dynamic_cast<const ExtensionGrantedEvent*>(&event)) {
        const std::string prefix = extension->wasUpdated() ? "Extension updated" : "Extension granted";
        Notification notification(0, extension->getStudentId(),
            prefix + "; new deadline: " + extension->getNewDeadline() + "; reason: " + extension->getReason(),
            m_clock.now(), false);
        m_notifications.save(notification);
        return;
    }
    const auto* created = dynamic_cast<const AssignmentCreatedEvent*>(&event);
    if (!created) return;
    const auto students = m_enrolments.getStudentsForCourse(created->getCourseId());
    const std::string message = "New assignment: " + created->getTitle() + ", due " + created->getLocalDeadline();
    for (const auto& student : students) {
        if (student->getRole() != Role::Student) continue;
        Notification notification(0, student->getId(), message, m_clock.now(), false);
        m_notifications.save(notification);
    }
}

std::vector<Notification> NotificationService::listUnread(const User& user) const {
    if (!user.canPerform(Action::ViewNotifications))
        throw AuthorisationException("User cannot view notifications");
    std::vector<Notification> unread;
    for (const auto& notification : m_notifications.getAll())
        if (notification.getUserId() == user.getId() && !notification.isRead()) unread.push_back(notification);
    return unread;
}

void NotificationService::markRead(const User& user, int notificationId) {
    if (!user.canPerform(Action::ViewNotifications))
        throw AuthorisationException("User cannot view notifications");
    auto notification = m_notifications.getById(notificationId);
    if (notification->getUserId() != user.getId())
        throw AuthorisationException("Cannot update another user's notification");
    notification->setRead(true);
    m_notifications.save(*notification);
}

} // namespace Lms
