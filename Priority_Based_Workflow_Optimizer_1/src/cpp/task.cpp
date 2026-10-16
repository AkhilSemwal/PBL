// task.cpp
// Owner: Akhil
//
// Part 1: the text helpers (pu_...) that every file reader uses - C style.
// Part 2: the code of the Task class - C++ / OOP.

#include "task.h"

#include <cstring>
#include <cstdio>
#include <climits>

// ============================ Part 1: text helpers ============================

int pu_read_line(FILE* file, char* buffer, int bufferSize, int* lineTooLong)
{
    *lineTooLong = 0;

    // fgets gives NULL when there is nothing left to read.
    if (fgets(buffer, bufferSize, file) == NULL) {
        return 0;
    }

    size_t length = strlen(buffer);

    // Normal case: the whole line fitted, so the last character is '\n'.
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
        return 1;
    }

    // No '\n' at the end: either this was the last line of the file,
    // or the line was longer than our buffer.
    if (feof(file)) {
        return 1;
    }

    *lineTooLong = 1;

    // Throw away the rest of the long line, so the next call starts
    // reading at the beginning of the next line.
    int character = fgetc(file);
    while (character != '\n' && character != EOF) {
        character = fgetc(file);
    }
    return 1;
}

// Small helper: is this a space, a tab or a line-end character?
static int is_space_character(char character)
{
    if (character == ' ' || character == '\t' || character == '\r' || character == '\n') {
        return 1;
    }
    return 0;
}

void pu_trim(char* text)
{
    size_t length = strlen(text);

    // First cut the spaces at the end by moving the '\0' backwards.
    while (length > 0 && is_space_character(text[length - 1])) {
        text[length - 1] = '\0';
        length--;
    }

    // Then count the spaces at the start...
    size_t start = 0;
    while (text[start] != '\0' && is_space_character(text[start])) {
        start++;
    }

    // ...and slide the rest of the text to the front.
    if (start > 0) {
        size_t position = 0;
        while (text[start + position] != '\0') {
            text[position] = text[start + position];
            position++;
        }
        text[position] = '\0';
    }
}

int pu_is_skippable(const char* line)
{
    // Skip the spaces at the start, then look at the first real character.
    size_t index = 0;
    while (line[index] != '\0' && is_space_character(line[index])) {
        index++;
    }
    if (line[index] == '\0') {
        return 1;   // the line is empty (or only spaces)
    }
    if (line[index] == '#') {
        return 1;   // a comment line
    }
    return 0;
}

int pu_split(char* line, char separator, char** fields, int maxFields)
{
    int fieldCount = 0;
    char* fieldStart = line;
    size_t index = 0;

    // Walk along the line. Every separator (and the end of the line)
    // closes one field: we put a '\0' there and remember where it started.
    while (1) {
        char current = line[index];
        if (current == separator || current == '\0') {
            line[index] = '\0';
            if (fieldCount < maxFields) {
                pu_trim(fieldStart);
                fields[fieldCount] = fieldStart;
            }
            fieldCount++;
            if (current == '\0') {
                break;   // this was the end of the line
            }
            fieldStart = &line[index + 1];   // the next field starts after the separator
        }
        index++;
    }
    return fieldCount;
}

int pu_parse_int(const char* text, int* value)
{
    size_t index = 0;
    int isNegative = 0;
    long result = 0;

    // An optional + or - sign at the start.
    if (text[0] == '-' || text[0] == '+') {
        isNegative = (text[0] == '-');
        index = 1;
    }

    // There must be at least one digit after the sign.
    if (text[index] == '\0') {
        return 0;
    }

    // Build the number digit by digit; anything that is not a digit is an error.
    while (text[index] != '\0') {
        char digit = text[index];
        if (digit < '0' || digit > '9') {
            return 0;
        }
        result = result * 10 + (digit - '0');
        if (result > INT_MAX) {
            return 0;   // too big to fit in an int
        }
        index++;
    }

    if (isNegative) {
        result = -result;
    }
    *value = (int)result;
    return 1;
}

int pu_parse_time(const char* text, int* minutes)
{
    // We accept two shapes:
    //   "13:50"     - 24-hour, 5 characters
    //   "01:50 PM"  - 12-hour, 8 characters (a space, then AM or PM)
    size_t length = strlen(text);
    int isTwelveHour = 0;
    int isPm = 0;

    if (length == 8) {
        // 12-hour form: check the space and the AM / PM (small letters are fine too).
        if (text[5] != ' ') {
            return 0;
        }
        char first = text[6];
        char second = text[7];
        if (second != 'M' && second != 'm') {
            return 0;
        }
        if (first == 'A' || first == 'a') {
            isTwelveHour = 1;
        } else if (first == 'P' || first == 'p') {
            isTwelveHour = 1;
            isPm = 1;
        } else {
            return 0;
        }
    } else if (length != 5) {
        return 0;
    }

    // The first five characters must look like HH:MM.
    if (text[2] != ':') {
        return 0;
    }
    if (text[0] < '0' || text[0] > '9' || text[1] < '0' || text[1] > '9') {
        return 0;
    }
    if (text[3] < '0' || text[3] > '9' || text[4] < '0' || text[4] > '9') {
        return 0;
    }

    // Turn the digit characters into numbers ('9' - '0' = 9).
    int hours = (text[0] - '0') * 10 + (text[1] - '0');
    int mins = (text[3] - '0') * 10 + (text[4] - '0');
    if (mins > 59) {
        return 0;
    }

    if (isTwelveHour) {
        // A 12-hour clock only shows 1 to 12.
        if (hours < 1 || hours > 12) {
            return 0;
        }
        // 12 is special: 12:30 AM is just after midnight (00:30),
        // and 12:30 PM is just after noon (12:30).
        if (hours == 12) {
            hours = 0;
        }
        if (isPm) {
            hours += 12;
        }
    } else if (hours > 23) {
        return 0;
    }

    *minutes = hours * 60 + mins;   // e.g. 09:30 -> 9 * 60 + 30 = 570
    return 1;
}

void pu_format_time(int minutes, char* buffer, int bufferSize)
{
    if (minutes < 0) {
        minutes = 0;   // never print a negative time
    }
    int hours = minutes / 60;
    int mins = minutes % 60;
    snprintf(buffer, (size_t)bufferSize, "%02d:%02d", hours, mins);   // 570 -> "09:30"
}

int pu_copy_text(char* destination, int destinationSize, const char* source)
{
    size_t length = strlen(source);
    if (destinationSize <= 0) {
        return 0;
    }
    // We need room for every character plus the '\0' at the end.
    // If it does not fit, we copy nothing, so we never write outside the array.
    if (length >= (size_t)destinationSize) {
        destination[0] = '\0';
        return 0;
    }
    memcpy(destination, source, length + 1);   // +1 copies the '\0' too
    return 1;
}

// Small helper: turns 'A'..'Z' into 'a'..'z' and leaves everything else as it is.
static char to_lower_character(char character)
{
    if (character >= 'A' && character <= 'Z') {
        return (char)(character - 'A' + 'a');
    }
    return character;
}

int pu_equals_ignore_case(const char* first, const char* second)
{
    size_t index = 0;
    while (first[index] != '\0' && second[index] != '\0') {
        if (to_lower_character(first[index]) != to_lower_character(second[index])) {
            return 0;
        }
        index++;
    }
    // Both texts must end at the same place, so "Lab" is not equal to "Library".
    return first[index] == '\0' && second[index] == '\0';
}

// ============================ Part 2: the Task class ============================

Task::Task()
{
    // Start with every field set to zero, and "no building" as the location.
    std::memset(&data, 0, sizeof(data));
    data.locationId = NO_LOCATION;
}

Task::Task(const TaskData& record)
    : data(record)   // keep our own copy of the record
{
}

// The getters only read the private record; none of them change it.

int Task::getId() const
{
    return data.id;
}

const char* Task::getName() const
{
    return data.name;
}

int Task::getDuration() const
{
    return data.durationMinutes;
}

int Task::getPriority() const
{
    return data.priority;
}

int Task::getLocationId() const
{
    return data.locationId;
}

int Task::getPrerequisiteCount() const
{
    return data.prerequisiteCount;
}

int Task::getPrerequisiteId(int position) const
{
    // Check the position first so we never read outside the array.
    if (position < 0 || position >= data.prerequisiteCount) {
        return -1;
    }
    return data.prerequisiteIds[position];
}

bool Task::isCompleted() const
{
    return data.isCompleted != 0;
}

bool Task::hasPrerequisite(int taskId) const
{
    for (int position = 0; position < data.prerequisiteCount; position++) {
        if (data.prerequisiteIds[position] == taskId) {
            return true;
        }
    }
    return false;
}

bool Task::ranksHigherThan(const Task& other) const
{
    // 1. The more important task wins.
    if (data.priority != other.data.priority) {
        return data.priority > other.data.priority;
    }
    // 2. Same priority: the shorter task wins (it is easier to fit in a slot).
    if (data.durationMinutes != other.data.durationMinutes) {
        return data.durationMinutes < other.data.durationMinutes;
    }
    // 3. Still equal: the lower ID wins, so the answer is always the same.
    return data.id < other.data.id;
}

bool Task::operator>(const Task& other) const
{
    return ranksHigherThan(other);
}

bool Task::operator==(const Task& other) const
{
    return data.id == other.data.id;
}

std::ostream& operator<<(std::ostream& out, const Task& task)
{
    out << "T" << task.getId() << " " << task.getName()
        << " (" << task.getDuration() << " min, P" << task.getPriority() << ")";
    return out;   // returning the stream lets us chain: cout << t1 << t2
}
