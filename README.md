# Smart Complaint and Grievance Management System

A C++17 console application that lets citizens, students or employees file complaints
(corruption, harassment, civic issues, public safety), assigns a priority automatically,
tracks each complaint from *Filed* to *Resolved* and reports how well an institution is
meeting its resolution target.

**Mapped SDG:** SDG 16 – Peace, Justice and Strong Institutions

## Features
- File complaints in four categories, with an optional **anonymous** mode
- Automatic priority (Low / Medium / High) from category and severity
- View all complaints, view sorted by priority, search by ID
- Update status (Filed → Under Review → Resolved) and remove complaints
- Statistics: status counts, category-wise report, resolution rate vs. a target
- Category-specific recommended actions (runtime polymorphism)
- Data saved to `complaints.txt` on exit and reloaded on the next run
- Input validation with custom exceptions

## OOP concepts used
| Concept | Where |
|---|---|
| Abstraction | Abstract class `Complaint` with pure virtual functions |
| Encapsulation | Protected data members, public getters and methods |
| Inheritance | `CorruptionComplaint`, `HarassmentComplaint`, `CivicComplaint`, `SafetyComplaint` |
| Polymorphism | `type()`, `department()`, `suggestedAction()`, `basePriority()` |
| Operator overloading | `operator<<` (print) and `operator<` (compare priority) |
| Exception handling | `InvalidInputException`, `InputEndedException` |
| STL and smart pointers | `vector<unique_ptr<Complaint>>`, `map`, `stable_sort`, `find_if` |
| File handling | `ifstream` / `ofstream` with a pipe-separated record format |

## Build and run
```
g++ -std=c++17 -Wall -Wextra -pedantic complaint_management.cpp -o complaint_tracker
./complaint_tracker
```
On Windows (MinGW): `complaint_tracker.exe`

## Priority rule
`score = basePriority(category) + severity - 1`  (range 1–5)
High ≥ 4, Medium = 3, Low ≤ 2. Base priorities: Civic 1, Corruption 2, Harassment 3, Safety 3.

## Authors
**Group 3I1** – BTECH, 3rd Semester, Object-Oriented Programming Using C++

- Mukul Singh (07714803125)
- Chris Smith (07114803125)
