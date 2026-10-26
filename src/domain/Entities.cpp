#include "Entities.h"

namespace Lms {

Department::Department(int id, const std::string& name)
    : m_id(id), m_name(name) {}

Course::Course(int id, const std::string& code, const std::string& title, int departmentId)
    : m_id(id), m_code(code), m_title(title), m_departmentId(departmentId) {}

Assignment::Assignment(int id, int courseId, const std::string& title,
                        const std::string& deadline, double maxMark, int createdBy)
    : m_id(id), m_courseId(courseId), m_title(title), m_deadline(deadline),
      m_maxMark(maxMark), m_createdBy(createdBy) {}

Extension::Extension(int id, int assignmentId, int studentId,
                     const std::string& newDeadline, const std::string& reason, int grantedBy)
    : m_id(id), m_assignmentId(assignmentId), m_studentId(studentId),
      m_newDeadline(newDeadline), m_reason(reason), m_grantedBy(grantedBy) {}

Submission::Submission(int id, int assignmentId, int studentId, SubmissionState state)
    : m_id(id), m_assignmentId(assignmentId), m_studentId(studentId), m_state(state) {}

SubmissionVersion::SubmissionVersion(int submissionId, int versionNo, const std::string& content,
                                      const std::string& submittedAt)
    : m_submissionId(submissionId), m_versionNo(versionNo), m_content(content),
      m_submittedAt(submittedAt) {}

MarkAuditEntry::MarkAuditEntry(int id, int submissionId, int changedBy,
                               const std::string& changedAt, double oldValue, double newValue,
                               const std::string& reason)
    : m_id(id), m_submissionId(submissionId), m_changedBy(changedBy),
      m_changedAt(changedAt), m_oldValue(oldValue), m_newValue(newValue),
      m_reason(reason) {}

Notification::Notification(int id, int userId, const std::string& message,
                           const std::string& createdAt, bool isRead)
    : m_id(id), m_userId(userId), m_message(message), m_createdAt(createdAt),
      m_isRead(isRead) {}

} // namespace Lms
