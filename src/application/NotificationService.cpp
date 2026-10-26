#include "NotificationService.h"
#include "domain/LmsException.h"

namespace Lms {

NotificationService::NotificationService(IEnrolmentRepository& enrolments,
        INotificationRepository& notifications, const Clock& clock)
    : m_enrolments(enrolments), m_notifications(notifications), m_clock(clock) {}

void NotificationService::onEvent(const Event& event) {
    if (const auto* assigned = dynamic_cast<const LecturerAssignedEvent*>(&event)) {
        const std::string message = "You have been assigned to course " + assigned->getCourseCode();
        Notification notification(0, assigned->getLecturerId(), message, m_clock.now(), false);
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

} // namespace Lms
