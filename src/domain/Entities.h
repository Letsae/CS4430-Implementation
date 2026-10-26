#pragma once

#include <string>
#include <vector>

namespace Lms {

/**
 * Department entity.
 */
class Department {
public:
    Department() = default;
    Department(int id, const std::string& name);

    int getId() const { return m_id; }
    const std::string& getName() const { return m_name; }

    void setId(int id) { m_id = id; }
    void setName(const std::string& name) { m_name = name; }

private:
    int m_id = 0;
    std::string m_name;
};

/**
 * Course entity.
 */
class Course {
public:
    Course() = default;
    Course(int id, const std::string& code, const std::string& title, int departmentId);

    int getId() const { return m_id; }
    const std::string& getCode() const { return m_code; }
    const std::string& getTitle() const { return m_title; }
    int getDepartmentId() const { return m_departmentId; }

    void setId(int id) { m_id = id; }
    void setCode(const std::string& code) { m_code = code; }
    void setTitle(const std::string& title) { m_title = title; }
    void setDepartmentId(int departmentId) { m_departmentId = departmentId; }

private:
    int m_id = 0;
    std::string m_code;
    std::string m_title;
    int m_departmentId = 0;
};

/**
 * Assignment entity.
 */
class Assignment {
public:
    Assignment() = default;
    Assignment(int id, int courseId, const std::string& title,
               const std::string& deadline, double maxMark, int createdBy);

    int getId() const { return m_id; }
    int getCourseId() const { return m_courseId; }
    const std::string& getTitle() const { return m_title; }
    const std::string& getDeadline() const { return m_deadline; }
    double getMaxMark() const { return m_maxMark; }
    int getCreatedBy() const { return m_createdBy; }

    void setId(int id) { m_id = id; }
    void setCourseId(int courseId) { m_courseId = courseId; }
    void setTitle(const std::string& title) { m_title = title; }
    void setDeadline(const std::string& deadline) { m_deadline = deadline; }
    void setMaxMark(double maxMark) { m_maxMark = maxMark; }
    void setCreatedBy(int createdBy) { m_createdBy = createdBy; }

private:
    int m_id = 0;
    int m_courseId = 0;
    std::string m_title;
    std::string m_deadline;
    double m_maxMark = 0.0;
    int m_createdBy = 0;
};

/**
 * Extension entity.
 */
class Extension {
public:
    Extension() = default;
    Extension(int id, int assignmentId, int studentId,
              const std::string& newDeadline, const std::string& reason, int grantedBy);

    int getId() const { return m_id; }
    int getAssignmentId() const { return m_assignmentId; }
    int getStudentId() const { return m_studentId; }
    const std::string& getNewDeadline() const { return m_newDeadline; }
    const std::string& getReason() const { return m_reason; }
    int getGrantedBy() const { return m_grantedBy; }

    void setId(int id) { m_id = id; }
    void setAssignmentId(int assignmentId) { m_assignmentId = assignmentId; }
    void setStudentId(int studentId) { m_studentId = studentId; }
    void setNewDeadline(const std::string& newDeadline) { m_newDeadline = newDeadline; }
    void setReason(const std::string& reason) { m_reason = reason; }
    void setGrantedBy(int grantedBy) { m_grantedBy = grantedBy; }

private:
    int m_id = 0;
    int m_assignmentId = 0;
    int m_studentId = 0;
    std::string m_newDeadline;
    std::string m_reason;
    int m_grantedBy = 0;
};

/**
 * Submission state enum.
 */
enum class SubmissionState {
    NotSubmitted,
    Submitted,
    Late,
    Marked,
    Moderated,
    Released
};

/**
 * Submission entity.
 */
class Submission {
public:
    Submission() = default;
    Submission(int id, int assignmentId, int studentId, SubmissionState state);

    int getId() const { return m_id; }
    int getAssignmentId() const { return m_assignmentId; }
    int getStudentId() const { return m_studentId; }
    SubmissionState getState() const { return m_state; }
    double getRawMark() const { return m_rawMark; }
    double getPenaltyPercent() const { return m_penaltyPercent; }
    double getFinalMark() const { return m_finalMark; }
    const std::string& getFeedback() const { return m_feedback; }
    const std::string& getFirstSubmittedAt() const { return m_firstSubmittedAt; }

    void setId(int id) { m_id = id; }
    void setAssignmentId(int assignmentId) { m_assignmentId = assignmentId; }
    void setStudentId(int studentId) { m_studentId = studentId; }
    void setState(SubmissionState state) { m_state = state; }
    void setRawMark(double rawMark) { m_rawMark = rawMark; }
    void setPenaltyPercent(double penaltyPercent) { m_penaltyPercent = penaltyPercent; }
    void setFinalMark(double finalMark) { m_finalMark = finalMark; }
    void setFeedback(const std::string& feedback) { m_feedback = feedback; }
    void setFirstSubmittedAt(const std::string& firstSubmittedAt) { m_firstSubmittedAt = firstSubmittedAt; }

private:
    int m_id = 0;
    int m_assignmentId = 0;
    int m_studentId = 0;
    SubmissionState m_state = SubmissionState::NotSubmitted;
    double m_rawMark = 0.0;
    double m_penaltyPercent = 0.0;
    double m_finalMark = 0.0;
    std::string m_feedback;
    std::string m_firstSubmittedAt;
};

/**
 * SubmissionVersion entity.
 */
class SubmissionVersion {
public:
    SubmissionVersion() = default;
    SubmissionVersion(int submissionId, int versionNo, const std::string& content,
                      const std::string& submittedAt);

    int getSubmissionId() const { return m_submissionId; }
    int getVersionNo() const { return m_versionNo; }
    const std::string& getContent() const { return m_content; }
    const std::string& getSubmittedAt() const { return m_submittedAt; }

    void setSubmissionId(int submissionId) { m_submissionId = submissionId; }
    void setVersionNo(int versionNo) { m_versionNo = versionNo; }
    void setContent(const std::string& content) { m_content = content; }
    void setSubmittedAt(const std::string& submittedAt) { m_submittedAt = submittedAt; }

private:
    int m_submissionId = 0;
    int m_versionNo = 0;
    std::string m_content;
    std::string m_submittedAt;
};

/**
 * MarkAuditEntry entity - append-only audit log for mark changes.
 */
class MarkAuditEntry {
public:
    MarkAuditEntry() = default;
    MarkAuditEntry(int id, int submissionId, int changedBy,
                   const std::string& changedAt, double oldValue, double newValue,
                   const std::string& reason);

    int getId() const { return m_id; }
    int getSubmissionId() const { return m_submissionId; }
    int getChangedBy() const { return m_changedBy; }
    const std::string& getChangedAt() const { return m_changedAt; }
    double getOldValue() const { return m_oldValue; }
    double getNewValue() const { return m_newValue; }
    const std::string& getReason() const { return m_reason; }

    void setId(int id) { m_id = id; }
    void setSubmissionId(int submissionId) { m_submissionId = submissionId; }
    void setChangedBy(int changedBy) { m_changedBy = changedBy; }
    void setChangedAt(const std::string& changedAt) { m_changedAt = changedAt; }
    void setOldValue(double oldValue) { m_oldValue = oldValue; }
    void setNewValue(double newValue) { m_newValue = newValue; }
    void setReason(const std::string& reason) { m_reason = reason; }

private:
    int m_id = 0;
    int m_submissionId = 0;
    int m_changedBy = 0;
    std::string m_changedAt;
    double m_oldValue = 0.0;
    double m_newValue = 0.0;
    std::string m_reason;
};

/**
 * Notification entity.
 */
class Notification {
public:
    Notification() = default;
    Notification(int id, int userId, const std::string& message,
                 const std::string& createdAt, bool isRead);

    int getId() const { return m_id; }
    int getUserId() const { return m_userId; }
    const std::string& getMessage() const { return m_message; }
    const std::string& getCreatedAt() const { return m_createdAt; }
    bool isRead() const { return m_isRead; }

    void setId(int id) { m_id = id; }
    void setUserId(int userId) { m_userId = userId; }
    void setMessage(const std::string& message) { m_message = message; }
    void setCreatedAt(const std::string& createdAt) { m_createdAt = createdAt; }
    void setRead(bool isRead) { m_isRead = isRead; }

private:
    int m_id = 0;
    int m_userId = 0;
    std::string m_message;
    std::string m_createdAt;
    bool m_isRead = false;
};

} // namespace Lms
