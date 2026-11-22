// ============================================================================
//  Smart Complaint and Grievance Management System   (SDG 16)
//  OOP Mini Project - C++17
//
//  Compile : g++ -std=c++17 -Wall -Wextra -pedantic complaint_management.cpp -o complaint_tracker
//  Run     : ./complaint_tracker
//
//  Concepts demonstrated
//    - Abstraction      : abstract class Complaint (pure virtual functions)
//    - Encapsulation    : protected data + public getters/methods
//    - Inheritance      : CorruptionComplaint, HarassmentComplaint,
//                         CivicComplaint, SafetyComplaint
//    - Polymorphism     : type(), department(), suggestedAction(), basePriority()
//    - Operator overload: operator<< (print) and operator< (compare priority)
//    - Exceptions       : InvalidInputException, InputEndedException
//    - STL / smart ptrs : vector<unique_ptr<Complaint>>, map, sort, find_if
//    - File handling    : complaints.txt (save on exit, load on start)
// ============================================================================

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// ---------------------------------------------------------------------------
//  Custom exceptions
// ---------------------------------------------------------------------------
class InvalidInputException : public runtime_error {
public:
    explicit InvalidInputException(const string& msg) : runtime_error(msg) {}
};

class InputEndedException : public runtime_error {
public:
    InputEndedException() : runtime_error("Input stream ended.") {}
};

// ---------------------------------------------------------------------------
//  Status of a complaint
// ---------------------------------------------------------------------------
enum class Status { Filed = 1, UnderReview = 2, Resolved = 3 };

string statusToString(Status s) {
    switch (s) {
        case Status::Filed:       return "Filed";
        case Status::UnderReview: return "Under Review";
        case Status::Resolved:    return "Resolved";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
//  Small helper functions
// ---------------------------------------------------------------------------
string today() {
    time_t t = time(nullptr);
    tm* lt = localtime(&t);
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", lt);
    return string(buf);
}

string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string clip(const string& s, size_t n) {
    return s.size() > n ? s.substr(0, n - 3) + "..." : s;
}

// The file uses '|' as a separator, so it must not appear inside user text.
string sanitize(string s) {
    replace(s.begin(), s.end(), '|', '/');
    replace(s.begin(), s.end(), '\n', ' ');
    return s;
}

vector<string> split(const string& line, char sep) {
    vector<string> parts;
    string item;
    stringstream ss(line);
    while (getline(ss, item, sep)) parts.push_back(item);
    if (!line.empty() && line.back() == sep) parts.push_back("");
    return parts;
}

// ---------------------------------------------------------------------------
//  Validated console input
// ---------------------------------------------------------------------------
string readLine(const string& prompt, bool allowEmpty = false) {
    string s;
    while (true) {
        cout << prompt;
        if (!getline(cin, s)) throw InputEndedException();
        s = trim(s);
        if (!s.empty() || allowEmpty) return s;
        cout << "  Input cannot be empty. Please try again.\n";
    }
}

int readInt(const string& prompt, int lo, int hi) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t pos = 0;
            int v = stoi(s, &pos);
            if (pos != s.size())
                throw InvalidInputException("Please enter a whole number only.");
            if (v < lo || v > hi)
                throw InvalidInputException("Please enter a number between " +
                                            to_string(lo) + " and " + to_string(hi) + ".");
            return v;
        } catch (const InvalidInputException& e) {
            cout << "  " << e.what() << "\n";
        } catch (const logic_error&) {   // stoi: invalid_argument / out_of_range
            cout << "  That is not a valid number. Please try again.\n";
        }
    }
}

double readDouble(const string& prompt, double lo, double hi) {
    while (true) {
        string s = readLine(prompt);
        try {
            size_t pos = 0;
            double v = stod(s, &pos);
            if (pos != s.size())
                throw InvalidInputException("Please enter a number only.");
            if (v < lo || v > hi)
                throw InvalidInputException("Please enter a value between " +
                                            to_string((int)lo) + " and " + to_string((int)hi) + ".");
            return v;
        } catch (const InvalidInputException& e) {
            cout << "  " << e.what() << "\n";
        } catch (const logic_error&) {
            cout << "  That is not a valid number. Please try again.\n";
        }
    }
}

bool readYesNo(const string& prompt) {
    while (true) {
        string s = readLine(prompt + " (y/n): ");
        if (s == "y" || s == "Y") return true;
        if (s == "n" || s == "N") return false;
        cout << "  Please type y or n.\n";
    }
}

// ---------------------------------------------------------------------------
//  Abstract base class
// ---------------------------------------------------------------------------
class Complaint {
protected:
    int    id;
    string title;
    string description;
    string location;
    string filer;        // "Anonymous" when the complainant wants protection
    bool   anonymous;
    int    severity;     // 1 (minor) .. 3 (critical), entered by the complainant
    string dateFiled;
    Status status;

public:
    Complaint(int id, string title, string description, string location,
              string filer, bool anonymous, int severity, string dateFiled,
              Status status = Status::Filed)
        : id(id), title(move(title)), description(move(description)),
          location(move(location)), filer(move(filer)), anonymous(anonymous),
          severity(severity), dateFiled(move(dateFiled)), status(status) {}

    virtual ~Complaint() = default;

    // ---- pure virtual functions: every category must supply its own ----
    virtual string type() const = 0;
    virtual string department() const = 0;
    virtual string suggestedAction() const = 0;
    virtual int    basePriority() const = 0;   // 1 (low) .. 3 (high)
    virtual int    typeCode() const = 0;       // used for the file format

    // ---- common behaviour ----
    int priorityScore() const { return basePriority() + severity - 1; }   // 1..5

    string priorityLevel() const {
        int s = priorityScore();
        if (s >= 4) return "High";
        if (s == 3) return "Medium";
        return "Low";
    }

    int    getId() const        { return id; }
    Status getStatus() const    { return status; }
    void   setStatus(Status s)  { status = s; }
    bool   isResolved() const   { return status == Status::Resolved; }
    const string& getTitle() const    { return title; }
    const string& getDate() const     { return dateFiled; }

    string serialize() const {
        ostringstream o;
        o << typeCode() << '|' << id << '|' << static_cast<int>(status) << '|'
          << (anonymous ? 1 : 0) << '|' << severity << '|' << dateFiled << '|'
          << sanitize(title) << '|' << sanitize(description) << '|'
          << sanitize(location) << '|' << sanitize(filer);
        return o.str();
    }

    // More urgent complaints compare as "greater".
    bool operator<(const Complaint& other) const {
        return priorityScore() < other.priorityScore();
    }

    friend ostream& operator<<(ostream& os, const Complaint& c) {
        os << "-------------------------------------------------\n"
           << " Complaint ID : " << c.id << "\n"
           << " Category     : " << c.type() << "\n"
           << " Title        : " << c.title << "\n"
           << " Description  : " << c.description << "\n"
           << " Location     : " << c.location << "\n"
           << " Filed by     : " << (c.anonymous ? "Anonymous (identity protected)" : c.filer) << "\n"
           << " Date filed   : " << c.dateFiled << "\n"
           << " Priority     : " << c.priorityLevel() << " (score " << c.priorityScore() << "/5)\n"
           << " Status       : " << statusToString(c.status) << "\n"
           << " Department   : " << c.department() << "\n"
           << " Next step    : " << c.suggestedAction() << "\n"
           << "-------------------------------------------------\n";
        return os;
    }
};

// ---------------------------------------------------------------------------
//  Derived classes (inheritance + runtime polymorphism)
// ---------------------------------------------------------------------------
class CorruptionComplaint : public Complaint {
public:
    using Complaint::Complaint;
    string type() const override       { return "Corruption"; }
    string department() const override { return "Anti-Corruption Bureau / Vigilance Cell"; }
    string suggestedAction() const override {
        return "Preserve evidence, forward to the vigilance cell and protect the complainant's identity.";
    }
    int basePriority() const override  { return 2; }
    int typeCode() const override      { return 1; }
};

class HarassmentComplaint : public Complaint {
public:
    using Complaint::Complaint;
    string type() const override       { return "Harassment"; }
    string department() const override { return "Internal Complaints Committee / Women's Helpline"; }
    string suggestedAction() const override {
        return "Ensure the complainant's safety, record a statement and begin a confidential inquiry.";
    }
    int basePriority() const override  { return 3; }
    int typeCode() const override      { return 2; }
};

class CivicComplaint : public Complaint {
public:
    using Complaint::Complaint;
    string type() const override       { return "Civic Issue"; }
    string department() const override { return "Municipal Corporation / Ward Office"; }
    string suggestedAction() const override {
        return "Log the issue with the ward office, assign a field officer and share a timeline.";
    }
    int basePriority() const override  { return 1; }
    int typeCode() const override      { return 3; }
};

class SafetyComplaint : public Complaint {
public:
    using Complaint::Complaint;
    string type() const override       { return "Public Safety"; }
    string department() const override { return "Police / Emergency Response Unit"; }
    string suggestedAction() const override {
        return "Alert the nearest police station immediately and inspect the location the same day.";
    }
    int basePriority() const override  { return 3; }
    int typeCode() const override      { return 4; }
};

// Factory: builds the right derived object from a type code.
unique_ptr<Complaint> createComplaint(int typeCode, int id, const string& title,
                                      const string& desc, const string& loc,
                                      const string& filer, bool anon, int sev,
                                      const string& date, Status st) {
    switch (typeCode) {
        case 1: return make_unique<CorruptionComplaint>(id, title, desc, loc, filer, anon, sev, date, st);
        case 2: return make_unique<HarassmentComplaint>(id, title, desc, loc, filer, anon, sev, date, st);
        case 3: return make_unique<CivicComplaint>(id, title, desc, loc, filer, anon, sev, date, st);
        case 4: return make_unique<SafetyComplaint>(id, title, desc, loc, filer, anon, sev, date, st);
        default: return nullptr;
    }
}

// ---------------------------------------------------------------------------
//  Manager class: holds all complaints and implements the features
// ---------------------------------------------------------------------------
class ComplaintManager {
    vector<unique_ptr<Complaint>> complaints;
    int    nextId     = 1001;
    double targetRate = 70.0;      // desired resolution rate in percent
    string fileName;

    vector<unique_ptr<Complaint>>::iterator findById(int id) {
        return find_if(complaints.begin(), complaints.end(),
                       [id](const unique_ptr<Complaint>& c) { return c->getId() == id; });
    }

    void printTableHeader() const {
        cout << left << setw(7) << "ID" << setw(15) << "Category" << setw(27) << "Title"
             << setw(10) << "Priority" << setw(14) << "Status" << "Filed on\n"
             << string(81, '-') << "\n";
    }

    void printRow(const Complaint& c) const {
        cout << left << setw(7) << c.getId() << setw(15) << c.type()
             << setw(27) << clip(c.getTitle(), 25) << setw(10) << c.priorityLevel()
             << setw(14) << statusToString(c.getStatus()) << c.getDate() << "\n";
    }

public:
    explicit ComplaintManager(string file) : fileName(move(file)) {}

    // ---------------- file handling ----------------
    void load() {
        ifstream in(fileName);
        if (!in) return;                       // first run: no file yet
        string line;
        int skipped = 0, loaded = 0;
        while (getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            vector<string> f = split(line, '|');
            try {
                if (f[0] == "TARGET" && f.size() == 2) { targetRate = stod(f[1]); continue; }
                if (f[0] == "NEXT" && f.size() == 2)   { nextId = stoi(f[1]);     continue; }
                if (f.size() != 10) throw InvalidInputException("bad record");
                int  tc   = stoi(f[0]);
                int  id   = stoi(f[1]);
                int  st   = stoi(f[2]);
                bool anon = stoi(f[3]) == 1;
                int  sev  = stoi(f[4]);
                if (st < 1 || st > 3 || sev < 1 || sev > 3)
                    throw InvalidInputException("bad value");
                auto c = createComplaint(tc, id, f[6], f[7], f[8], f[9], anon, sev, f[5],
                                         static_cast<Status>(st));
                if (!c) throw InvalidInputException("unknown type");
                complaints.push_back(move(c));
                ++loaded;
            } catch (const exception&) {
                ++skipped;
            }
        }
        cout << "Loaded " << loaded << " saved complaint(s) from " << fileName << ".\n";
        if (skipped > 0) cout << "Warning: " << skipped << " damaged line(s) were skipped.\n";
    }

    void save() const {
        ofstream out(fileName);
        if (!out) {
            cout << "Error: could not write to " << fileName << ".\n";
            return;
        }
        out << "TARGET|" << targetRate << "\n";
        out << "NEXT|" << nextId << "\n";
        for (const auto& c : complaints) out << c->serialize() << "\n";
    }

    // ---------------- features ----------------
    void addComplaint() {
        cout << "\nComplaint category:\n"
             << "  1 Corruption   2 Harassment   3 Civic Issue   4 Public Safety\n";
        int tc = readInt("Category (1-4): ", 1, 4);
        string title = readLine("Short title: ");
        string desc  = readLine("Describe the issue: ");
        string loc   = readLine("Location / office concerned: ");
        bool   anon  = readYesNo("File anonymously?");
        string filer = "Anonymous";
        if (!anon) filer = readLine("Your name: ");
        cout << "Severity: 1 Minor, 2 Serious, 3 Critical\n";
        int sev = readInt("Severity (1-3): ", 1, 3);

        auto c = createComplaint(tc, nextId, title, desc, loc, filer, anon, sev,
                                 today(), Status::Filed);
        cout << "\nComplaint filed successfully.\n"
             << "Your complaint ID is " << nextId << " (note it to track progress).\n"
             << "Priority assigned: " << c->priorityLevel() << "\n";
        ++nextId;
        complaints.push_back(move(c));
    }

    void viewAll() const {
        if (complaints.empty()) { cout << "\nNo complaints recorded yet.\n"; return; }
        cout << "\n";
        printTableHeader();
        for (const auto& c : complaints) printRow(*c);
    }

    void viewByPriority() const {
        if (complaints.empty()) { cout << "\nNo complaints recorded yet.\n"; return; }
        vector<const Complaint*> list;
        for (const auto& c : complaints) list.push_back(c.get());
        // uses the overloaded operator<  (most urgent first)
        stable_sort(list.begin(), list.end(),
                    [](const Complaint* a, const Complaint* b) { return *b < *a; });
        cout << "\nComplaints sorted by priority (most urgent first):\n";
        printTableHeader();
        for (const Complaint* c : list) printRow(*c);
    }

    void searchById() const {
        if (complaints.empty()) { cout << "\nNo complaints recorded yet.\n"; return; }
        int id = readInt("Enter complaint ID: ", 1, 999999);
        auto it = find_if(complaints.begin(), complaints.end(),
                          [id](const unique_ptr<Complaint>& c) { return c->getId() == id; });
        if (it == complaints.end()) cout << "No complaint found with ID " << id << ".\n";
        else cout << "\n" << **it;           // operator<< on the polymorphic object
    }

    void updateStatus() {
        if (complaints.empty()) { cout << "\nNo complaints recorded yet.\n"; return; }
        int id = readInt("Enter complaint ID to update: ", 1, 999999);
        auto it = findById(id);
        if (it == complaints.end()) { cout << "No complaint found with ID " << id << ".\n"; return; }
        cout << "Current status: " << statusToString((*it)->getStatus()) << "\n"
             << "  1 Filed   2 Under Review   3 Resolved\n";
        int s = readInt("New status (1-3): ", 1, 3);
        (*it)->setStatus(static_cast<Status>(s));
        cout << "Complaint " << id << " is now: " << statusToString((*it)->getStatus()) << ".\n";
    }

    void removeComplaint() {
        if (complaints.empty()) { cout << "\nNo complaints recorded yet.\n"; return; }
        int id = readInt("Enter complaint ID to remove: ", 1, 999999);
        auto it = findById(id);
        if (it == complaints.end()) { cout << "No complaint found with ID " << id << ".\n"; return; }
        if (readYesNo("Delete complaint " + to_string(id) + "?")) {
            complaints.erase(it);
            cout << "Complaint removed.\n";
        } else {
            cout << "Deletion cancelled.\n";
        }
    }

    void setTarget() {
        cout << "\nCurrent target resolution rate: " << fixed << setprecision(1)
             << targetRate << "%\n";
        targetRate = readDouble("New target (0-100%): ", 0, 100);
        cout << "Target updated to " << targetRate << "%.\n";
    }

    void statistics() const {
        if (complaints.empty()) { cout << "\nNo complaints recorded yet.\n"; return; }

        int total = static_cast<int>(complaints.size());
        int resolved = 0, review = 0, filed = 0, urgentPending = 0;
        map<string, int> perCategory, resolvedPerCategory;
        map<string, string> actions;               // category -> suggested action

        for (const auto& c : complaints) {
            ++perCategory[c->type()];
            actions[c->type()] = c->suggestedAction();   // polymorphic call
            switch (c->getStatus()) {
                case Status::Resolved:    ++resolved; ++resolvedPerCategory[c->type()]; break;
                case Status::UnderReview: ++review;   break;
                case Status::Filed:       ++filed;    break;
            }
            if (!c->isResolved() && c->priorityLevel() == "High") ++urgentPending;
        }

        double rate = 100.0 * resolved / total;
        cout << fixed << setprecision(1);
        cout << "\nTotal complaints     : " << total << "\n"
             << "  Filed              : " << filed << "\n"
             << "  Under Review       : " << review << "\n"
             << "  Resolved           : " << resolved << "\n"
             << "Resolution rate      : " << rate << "%\n"
             << "High priority pending: " << urgentPending << "\n\n"
             << "Category-wise report:\n";
        for (const auto& p : perCategory)
            cout << "  " << left << setw(14) << p.first << ": " << p.second
                 << " complaint(s), " << resolvedPerCategory[p.first] << " resolved\n";

        cout << "\nTarget resolution rate: " << targetRate << "%\n";
        if (rate >= targetRate)
            cout << "Status: TARGET MET (" << rate - targetRate << "% above target).\n";
        else
            cout << "Status: BELOW target by " << targetRate - rate << "%.\n";

        cout << "\nRecommended actions for categories in the system:\n";
        for (const auto& a : actions)
            cout << "- " << a.first << ": " << a.second << "\n";
    }
};

// ---------------------------------------------------------------------------
//  Menu
// ---------------------------------------------------------------------------
void showMenu() {
    cout << "\n=== SMART COMPLAINT AND GRIEVANCE MANAGEMENT SYSTEM ===\n"
         << "1 File a complaint\n"
         << "2 View all complaints\n"
         << "3 View complaints by priority\n"
         << "4 Search complaint by ID\n"
         << "5 Update complaint status\n"
         << "6 Remove a complaint\n"
         << "7 Statistics and resolution report\n"
         << "8 Set resolution target\n"
         << "9 Save and exit\n";
}

int main() {
    ComplaintManager manager("complaints.txt");
    manager.load();

    try {
        bool running = true;
        while (running) {
            showMenu();
            int choice = readInt("Choice: ", 1, 9);
            switch (choice) {
                case 1: manager.addComplaint();    break;
                case 2: manager.viewAll();         break;
                case 3: manager.viewByPriority();  break;
                case 4: manager.searchById();      break;
                case 5: manager.updateStatus();    break;
                case 6: manager.removeComplaint(); break;
                case 7: manager.statistics();      break;
                case 8: manager.setTarget();       break;
                case 9: running = false;           break;
            }
        }
    } catch (const InputEndedException&) {
        cout << "\nInput closed. Saving records before exit.\n";
    }

    manager.save();
    cout << "Records saved. Goodbye!\n";
    return 0;
}
