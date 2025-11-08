#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <queue>
#include <chrono>
#include <random>
#include <algorithm>
#include <iomanip>
#include <limits>

const int TANK_REQ = 1;
const int HEALER_REQ = 1;
const int DPS_REQ = 3;

struct Player {
    int id;
};

int n_instances;
int parties_remaining;
int parties_formed;

std::queue<Player> tank_queue;
std::queue<Player> healer_queue;
std::queue<Player> dps_queue;

std::vector<bool> instance_status;
std::vector<int> instance_parties_served;
std::vector<double> instance_time_served;

std::mutex mtx;
std::mutex cout_mtx;
std::condition_variable cv;

std::random_device rd;
std::mt19937 gen;

void print_safe_status(const std::string& message, const std::vector<bool>& status_copy) {
    std::lock_guard<std::mutex> lock(cout_mtx);
    std::cout << "\n[INFO] " << message << "\n";
    std::cout << "Instance Status: |";
    for (std::vector<bool>::size_type i = 0; i < status_copy.size(); ++i) {
        std::cout << " I-" << i << ": " << (status_copy[i] ? "active" : "empty") << " |";
    }
    std::cout << "\n--------------------------------------------------------\n";
}

int get_random_time(int t1, int t2) {
    std::uniform_int_distribution<> distrib(t1, t2);
    return distrib(gen);
}

int get_validated_int(const std::string& prompt, int min_value) {
    int value;
    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            if (value >= min_value) {
                break;
            } else {
                std::cout << "Input must be " << min_value << " or greater. Please try again.\n";
            }
        } else {
            std::cout << "Invalid input. Please enter a number.\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return value;
}

void instance_task(int instance_id, int t1, int t2) {
    std::string info_message;
    std::vector<bool> status_snapshot;
    int my_party_id;

    while (true) {
        Player tank;
        Player healer;
        std::vector<Player> dps_group;
        dps_group.reserve(DPS_REQ);

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] {
                bool players_ready = tank_queue.size() >= TANK_REQ &&
                                     healer_queue.size() >= HEALER_REQ &&
                                     dps_queue.size() >= DPS_REQ;
                return (parties_remaining > 0 && players_ready) || (parties_remaining <= 0);
            });
            if (parties_remaining <= 0) {
                break;
            }

            my_party_id = parties_formed;
            parties_formed++;
            parties_remaining--;

            tank = tank_queue.front(); tank_queue.pop();
            healer = healer_queue.front(); healer_queue.pop();
            for (int i = 0; i < DPS_REQ; ++i) {
                dps_group.push_back(dps_queue.front());
                dps_queue.pop();
            }

            instance_status[instance_id] = true;

            info_message = "Party " + std::to_string(my_party_id) + " formed (T:" + std::to_string(tank.id) +
                           ", H:" + std::to_string(healer.id) +
                           ", D:" + std::to_string(dps_group[0].id) +
                           "," + std::to_string(dps_group[1].id) +
                           "," + std::to_string(dps_group[2].id) +
                           "). Entering instance " + std::to_string(instance_id);
            status_snapshot = instance_status;
        }

        print_safe_status(info_message, status_snapshot);
        int run_time = get_random_time(t1, t2);
        std::this_thread::sleep_for(std::chrono::seconds(run_time));

        {
            std::unique_lock<std::mutex> lock(mtx);
            instance_status[instance_id] = false;
            instance_parties_served[instance_id]++;
            instance_time_served[instance_id] += run_time;

            info_message = "Party " + std::to_string(my_party_id) + " FINISHED instance " + std::to_string(instance_id) +
                           " (Run time: " + std::to_string(run_time) + "s)";
            status_snapshot = instance_status;
            cv.notify_all();
        }

        print_safe_status(info_message, status_snapshot);
    }
}

int main() {
    int t_total, h_total, d_total, t1, t2;

    std::cout << "--- LFG Dungeon Queue Simulator ---\n";
    n_instances = get_validated_int("Enter max concurrent instances (n): ", 1);
    t_total = get_validated_int("Enter number of Tanks in queue (t): ", 0);
    h_total = get_validated_int("Enter number of Healers in queue (h): ", 0);
    d_total = get_validated_int("Enter number of DPS in queue (d): ", 0);
    t1 = get_validated_int("Enter min dungeon time (t1): ", 1);
    t2 = get_validated_int("Enter max dungeon time (t2) [" + std::to_string(t1) + " or more]: ", t1);

    int player_id_counter = 0;
    for (int i = 0; i < t_total; ++i) tank_queue.push(Player{player_id_counter++});
    for (int i = 0; i < h_total; ++i) healer_queue.push(Player{player_id_counter++});
    for (int i = 0; i < d_total; ++i) dps_queue.push(Player{player_id_counter++});

    parties_formed = 0;
    int max_possible_parties = std::min({(int)tank_queue.size() / TANK_REQ,
                                         (int)healer_queue.size() / HEALER_REQ,
                                         (int)dps_queue.size() / DPS_REQ});
    parties_remaining = max_possible_parties;

    instance_status.resize(n_instances, false);
    instance_parties_served.resize(n_instances, 0);
    instance_time_served.resize(n_instances, 0.0);
    gen.seed(rd());

    std::cout << "\n========================================================";
    std::cout << "\nQueue Open. Max possible parties: " << max_possible_parties;
    std::cout << "\nCreating " << n_instances << " worker threads (instances)...";
    std::cout << "\nTotal Tanks: " << tank_queue.size();
    std::cout << " | Total Healers: " << healer_queue.size();
    std::cout << " | Total DPS: " << dps_queue.size();
    std::cout << "\n========================================================\n";

    print_safe_status("Initial Status", instance_status);

    std::vector<std::thread> instance_threads;
    for (int i = 0; i < n_instances; ++i)
        instance_threads.emplace_back(instance_task, i, t1, t2);

    for (auto& th : instance_threads) th.join();

    {
        std::lock_guard<std::mutex> lock(cout_mtx);
        std::cout << "\n\n========================================================";
        std::cout << "\nAll parties have completed. Queue closed.";
        std::cout << "\n\n--- FINAL SIMULATION SUMMARY ---";
        std::cout << "\nTotal Parties Formed: " << parties_formed;
        std::cout << "\nPlayers left in queue:";
        std::cout << "\n  Tanks: " << tank_queue.size();
        std::cout << "\n  Healers: " << healer_queue.size();
        std::cout << "\n  DPS: " << dps_queue.size();
        std::cout << "\n\n--- Instance Statistics ---";
        std::cout << std::fixed << std::setprecision(2);
        for (int i = 0; i < n_instances; ++i) {
            std::cout << "\nInstance " << i << ":";
            std::cout << "\n  Total Parties Served: " << instance_parties_served[i];
            std::cout << "\n  Total Time Active: " << instance_time_served[i] << "s";
        }
        std::cout << "\n========================================================\n";
    }

    return 0;
}
