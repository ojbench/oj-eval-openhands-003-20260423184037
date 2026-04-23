
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <iomanip>

using namespace std;

struct Submission {
    string problem_name;
    string status;
    int time;
};

struct ProblemInfo {
    bool solved = false;
    int solve_time = 0;
    int incorrect_attempts = 0;

    bool frozen = false;
    int incorrect_before_freeze = 0;
    int submissions_after_freeze = 0;
    bool has_ac_after_freeze = false;
    int first_ac_time_after_freeze = 0;
    int incorrect_after_freeze_before_ac = 0;
};

struct Team {
    string name;
    int solved_count = 0;
    int total_penalty = 0;
    vector<int> solve_times; // Sorted descending
    vector<ProblemInfo> problems;
    vector<Submission> all_submissions;
    int last_flush_rank = 0;

    void update_solve_times() {
        solve_times.clear();
        for (const auto& p : problems) {
            if (p.solved) {
                solve_times.push_back(p.solve_time);
            }
        }
        sort(solve_times.rbegin(), solve_times.rend());
    }
};

bool compareTeams(const Team* a, const Team* b) {
    if (a->solved_count != b->solved_count)
        return a->solved_count > b->solved_count;
    if (a->total_penalty != b->total_penalty)
        return a->total_penalty < b->total_penalty;
    for (size_t i = 0; i < a->solve_times.size(); ++i) {
        if (a->solve_times[i] != b->solve_times[i])
            return a->solve_times[i] < b->solve_times[i];
    }
    return a->name < b->name;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string cmd;
    bool started = false;
    bool frozen_state = false;
    int duration = 0;
    int problem_count = 0;
    map<string, Team> teams_map;
    vector<string> team_names_ordered;
    vector<Team*> scoreboard;

    while (cin >> cmd) {
        if (cmd == "ADDTEAM") {
            string name;
            cin >> name;
            if (started) {
                cout << "[Error]Add failed: competition has started." << endl;
            } else if (teams_map.count(name)) {
                cout << "[Error]Add failed: duplicated team name." << endl;
            } else {
                teams_map[name].name = name;
                team_names_ordered.push_back(name);
                cout << "[Info]Add successfully." << endl;
            }
        } else if (cmd == "START") {
            string dummy;
            cin >> dummy >> duration >> dummy >> problem_count;
            if (started) {
                cout << "[Error]Start failed: competition has started." << endl;
            } else {
                started = true;
                for (auto& name : team_names_ordered) {
                    auto& team = teams_map[name];
                    team.problems.resize(problem_count);
                    scoreboard.push_back(&team);
                }
                // Initial sort by name
                sort(scoreboard.begin(), scoreboard.end(), [](const Team* a, const Team* b) {
                    return a->name < b->name;
                });
                for (int i = 0; i < scoreboard.size(); ++i) {
                    scoreboard[i]->last_flush_rank = i + 1;
                }
                cout << "[Info]Competition starts." << endl;
            }
        } else if (cmd == "SUBMIT") {
            string prob_name, dummy, team_name, status;
            int time;
            cin >> prob_name >> dummy >> team_name >> dummy >> status >> dummy >> time;
            int prob_idx = prob_name[0] - 'A';
            auto& team = teams_map[team_name];
            team.all_submissions.push_back({prob_name, status, time});

            auto& p = team.problems[prob_idx];
            if (!p.solved) {
                if (frozen_state) {
                    p.frozen = true;
                    p.submissions_after_freeze++;
                    if (status == "Accepted") {
                        if (!p.has_ac_after_freeze) {
                            p.has_ac_after_freeze = true;
                            p.first_ac_time_after_freeze = time;
                            p.incorrect_after_freeze_before_ac = p.submissions_after_freeze - 1;
                        }
                    }
                } else {
                    if (status == "Accepted") {
                        p.solved = true;
                        p.solve_time = time;
                        team.solved_count++;
                        team.total_penalty += 20 * p.incorrect_attempts + time;
                        team.update_solve_times();
                    } else {
                        p.incorrect_attempts++;
                    }
                }
            }
        } else if (cmd == "FLUSH") {
            sort(scoreboard.begin(), scoreboard.end(), compareTeams);
            for (int i = 0; i < scoreboard.size(); ++i) {
                scoreboard[i]->last_flush_rank = i + 1;
            }
            cout << "[Info]Flush scoreboard." << endl;
        } else if (cmd == "FREEZE") {
            if (frozen_state) {
                cout << "[Error]Freeze failed: scoreboard has been frozen." << endl;
            } else {
                frozen_state = true;
                for (auto& team_ptr : scoreboard) {
                    for (auto& p : team_ptr->problems) {
                        p.incorrect_before_freeze = p.incorrect_attempts;
                        p.submissions_after_freeze = 0;
                        p.has_ac_after_freeze = false;
                    }
                }
                cout << "[Info]Freeze scoreboard." << endl;
            }
        } else if (cmd == "SCROLL") {
            if (!frozen_state) {
                cout << "[Error]Scroll failed: scoreboard has not been frozen." << endl;
            } else {
                cout << "[Info]Scroll scoreboard." << endl;
                // Flush first
                sort(scoreboard.begin(), scoreboard.end(), compareTeams);
                auto print_scoreboard = [&]() {
                    for (int i = 0; i < scoreboard.size(); ++i) {
                        Team* t = scoreboard[i];
                        cout << t->name << " " << (i + 1) << " " << t->solved_count << " " << t->total_penalty;
                        for (int j = 0; j < problem_count; ++j) {
                            auto& p = t->problems[j];
                            cout << " ";
                            if (p.frozen) {
                                cout << "-" << p.incorrect_before_freeze << "/" << p.submissions_after_freeze;
                            } else if (p.solved) {
                                cout << "+";
                                if (p.incorrect_attempts > 0) cout << p.incorrect_attempts;
                            } else {
                                if (p.incorrect_attempts > 0) cout << "-" << p.incorrect_attempts;
                                else cout << ".";
                            }
                        }
                        cout << endl;
                    }
                };
                print_scoreboard();

                while (true) {
                    int target_idx = -1;
                    for (int i = scoreboard.size() - 1; i >= 0; --i) {
                        bool has_frozen = false;
                        for (int j = 0; j < problem_count; ++j) {
                            if (scoreboard[i]->problems[j].frozen) {
                                has_frozen = true;
                                break;
                            }
                        }
                        if (has_frozen) {
                            target_idx = i;
                            break;
                        }
                    }
                    if (target_idx == -1) break;

                    Team* t = scoreboard[target_idx];
                    int prob_idx = -1;
                    for (int j = 0; j < problem_count; ++j) {
                        if (t->problems[j].frozen) {
                            prob_idx = j;
                            break;
                        }
                    }

                    auto& p = t->problems[prob_idx];
                    p.frozen = false;
                    if (p.has_ac_after_freeze) {
                        p.solved = true;
                        p.solve_time = p.first_ac_time_after_freeze;
                        p.incorrect_attempts = p.incorrect_before_freeze + p.incorrect_after_freeze_before_ac;
                        t->solved_count++;
                        t->total_penalty += 20 * p.incorrect_attempts + p.solve_time;
                        t->update_solve_times();

                        // Move team up
                        int new_idx = target_idx;
                        while (new_idx > 0 && compareTeams(t, scoreboard[new_idx - 1])) {
                            new_idx--;
                        }
                        if (new_idx < target_idx) {
                            Team* replaced = scoreboard[new_idx];
                            for (int k = target_idx; k > new_idx; --k) {
                                scoreboard[k] = scoreboard[k - 1];
                            }
                            scoreboard[new_idx] = t;
                            cout << t->name << " " << replaced->name << " " << t->solved_count << " " << t->total_penalty << endl;
                        }
                    } else {
                        p.incorrect_attempts = p.incorrect_before_freeze + p.submissions_after_freeze;
                    }
                }
                print_scoreboard();
                frozen_state = false;
                for (int i = 0; i < scoreboard.size(); ++i) {
                    scoreboard[i]->last_flush_rank = i + 1;
                }
            }
        } else if (cmd == "QUERY_RANKING") {
            string name;
            cin >> name;
            if (!teams_map.count(name)) {
                cout << "[Error]Query ranking failed: cannot find the team." << endl;
            } else {
                cout << "[Info]Complete query ranking." << endl;
                if (frozen_state) {
                    cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled." << endl;
                }
                cout << name << " NOW AT RANKING " << teams_map[name].last_flush_rank << endl;
            }
        } else if (cmd == "QUERY_SUBMISSION") {
            string name, dummy, prob_cond, status_cond;
            cin >> name >> dummy >> prob_cond >> dummy >> status_cond;
            prob_cond = prob_cond.substr(8); // PROBLEM=...
            status_cond = status_cond.substr(7); // STATUS=...
            if (!teams_map.count(name)) {
                cout << "[Error]Query submission failed: cannot find the team." << endl;
            } else {
                cout << "[Info]Complete query submission." << endl;
                auto& team = teams_map[name];
                bool found = false;
                for (int i = team.all_submissions.size() - 1; i >= 0; --i) {
                    const auto& s = team.all_submissions[i];
                    if ((prob_cond == "ALL" || s.problem_name == prob_cond) &&
                        (status_cond == "ALL" || s.status == status_cond)) {
                        cout << name << " " << s.problem_name << " " << s.status << " " << s.time << endl;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    cout << "Cannot find any submission." << endl;
                }
            }
        } else if (cmd == "END") {
            cout << "[Info]Competition ends." << endl;
            break;
        }
    }

    return 0;
}
