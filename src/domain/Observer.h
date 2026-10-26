#pragma once

#include <string>
#include <memory>
#include <vector>
#include <utility>

namespace Lms {

/**
 * Base event class.
 */
class Event {
public:
    virtual ~Event() = default;
    virtual std::string getType() const = 0;
};

/**
 * Observer interface for the Observer pattern.
 */
class IObserver {
public:
    virtual ~IObserver() = default;

    /**
     * Handle an event.
     */
    virtual void onEvent(const Event& event) = 0;
};

/**
 * Event publisher that notifies observers of events.
 */
class EventPublisher {
public:
    /**
     * Subscribe an observer to events.
     */
    void subscribe(std::shared_ptr<IObserver> observer);

    /**
     * Unsubscribe an observer.
     */
    void unsubscribe(std::shared_ptr<IObserver> observer);

    /**
     * Publish an event to all subscribers.
     */
    void publish(const Event& event);

private:
    std::vector<std::weak_ptr<IObserver>> m_observers;
};

/**
 * Specific event types.
 */
class LateSubmissionEvent : public Event {
public:
    LateSubmissionEvent(int submissionId, int studentId, int assignmentId)
        : m_submissionId(submissionId), m_studentId(studentId), m_assignmentId(assignmentId) {}

    std::string getType() const override { return "LateSubmission"; }

    int getSubmissionId() const { return m_submissionId; }
    int getStudentId() const { return m_studentId; }
    int getAssignmentId() const { return m_assignmentId; }

private:
    int m_submissionId;
    int m_studentId;
    int m_assignmentId;
};

class MarksReleasedEvent : public Event {
public:
    MarksReleasedEvent(int courseId, int departmentId)
        : m_courseId(courseId), m_departmentId(departmentId) {}

    std::string getType() const override { return "MarksReleased"; }

    int getCourseId() const { return m_courseId; }
    int getDepartmentId() const { return m_departmentId; }

private:
    int m_courseId;
    int m_departmentId;
};

class MarkChangedEvent : public Event {
public:
    MarkChangedEvent(int submissionId, int changedBy, double oldValue, double newValue)
        : m_submissionId(submissionId), m_changedBy(changedBy),
          m_oldValue(oldValue), m_newValue(newValue) {}

    std::string getType() const override { return "MarkChanged"; }

    int getSubmissionId() const { return m_submissionId; }
    int getChangedBy() const { return m_changedBy; }
    double getOldValue() const { return m_oldValue; }
    double getNewValue() const { return m_newValue; }

private:
    int m_submissionId;
    int m_changedBy;
    double m_oldValue;
    double m_newValue;
};

class AssignmentCreatedEvent : public Event {
public:
    AssignmentCreatedEvent(int assignmentId, int courseId, std::string title, std::string localDeadline)
        : m_assignmentId(assignmentId), m_courseId(courseId), m_title(std::move(title)),
          m_localDeadline(std::move(localDeadline)) {}
    std::string getType() const override { return "AssignmentCreated"; }
    int getAssignmentId() const { return m_assignmentId; }
    int getCourseId() const { return m_courseId; }
    const std::string& getTitle() const { return m_title; }
    const std::string& getLocalDeadline() const { return m_localDeadline; }
private:
    int m_assignmentId;
    int m_courseId;
    std::string m_title;
    std::string m_localDeadline;
};

class LecturerAssignedEvent : public Event {
public:
    LecturerAssignedEvent(int lecturerId, int courseId, std::string courseCode)
        : m_lecturerId(lecturerId), m_courseId(courseId), m_courseCode(std::move(courseCode)) {}
    std::string getType() const override { return "LecturerAssigned"; }
    int getLecturerId() const { return m_lecturerId; }
    int getCourseId() const { return m_courseId; }
    const std::string& getCourseCode() const { return m_courseCode; }
private:
    int m_lecturerId;
    int m_courseId;
    std::string m_courseCode;
};

} // namespace Lms
