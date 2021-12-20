#include <iostream>
#include <stdexcept>
#include <random>
#include <ctime>
#include <vector>

#include "Parameter.hpp"
#include "MySimulator.hpp"

size_t SIMU_BOUND_SIZE = 4; // 4km^2 range

void ShowStatus(SimulationParameter &status, size_t mode) {
  if (mode == 1) {
    for (size_t i = 0; i < status.m_total_num_people; ++i) {
      std::cout << "===== Person info =====" << std::endl;
      std::cout << "Person " << status.g_person_status[i].m_index << ": (" << status.g_person_status[i].m_coordinate.first << ", " << status.g_person_status[i].m_coordinate.second << ")" << std::endl;
      std::cout << "move direction vector: (" << status.g_person_status[i].m_direction_vector.first << ", " << status.g_person_status[i].m_direction_vector.second << ")" << std::endl;
      std::cout << "move speed: " << status.g_person_status[i].m_move_speed << std::endl;
      std::cout << "status: " << status.g_person_status[i].m_status << std::endl;
    }
  }
  std::cout << "===== Global info =====" << std::endl;
  std::cout << "Total people: " << status.m_total_num_people << std::endl;
  std::cout << "Health people: " << status.g_num_health << std::endl;
  std::cout << "Infected people: " << status.g_num_infected << std::endl;
  std::cout << "Dead people: " << status.g_num_dead << std::endl;
  std::cout << "Recovered people: " << status.g_num_recovered << std::endl;
}

// after moving, check the boundary and update the moving direction if needed.
void UpdateDirection(struct PersonStatus &p_stat, struct myBoundary &my_bound) {
  // x coordinate out of bound (x < left_x)
  if (p_stat.m_coordinate.first < my_bound.left_x) {
    p_stat.m_coordinate.first = 2 * my_bound.left_x - p_stat.m_coordinate.first;
    p_stat.m_direction_vector.first = -1 * p_stat.m_direction_vector.first;
  }
  // x > right_x
  else if (p_stat.m_coordinate.first > my_bound.right_x) {
    p_stat.m_coordinate.first = 2 * my_bound.right_x - p_stat.m_coordinate.first;
    p_stat.m_direction_vector.first = -1 * p_stat.m_direction_vector.first;
  }
  // y coordinate out of bound (y < down_y)
  if (p_stat.m_coordinate.second < my_bound.down_y) {
    p_stat.m_coordinate.second = 2 * my_bound.down_y - p_stat.m_coordinate.second;
    p_stat.m_direction_vector.second = -1 * p_stat.m_direction_vector.second;
  }
  // y > up_y
  else if (p_stat.m_coordinate.second > my_bound.up_y) {
    p_stat.m_coordinate.second = 2 * my_bound.up_y - p_stat.m_coordinate.second;
    p_stat.m_direction_vector.second = -1 * p_stat.m_direction_vector.second;
  }
}

// update the location and move direction of everyone after moving once.
void Move(SimulationParameter &status) {
  // reset dirty_bit bit
  status.dirty_bit = 0;
  for (size_t i = 0; i < status.m_total_num_people; ++i) {
    // if dead, don't move
    if (status.g_person_status[i].m_status == 3) {
      continue;
    }
    // move 1 m per step, boundary = 2km : 2, step = 1m : 0.001
    status.g_person_status[i].m_coordinate.first += status.g_person_status[i].m_direction_vector.first * status.g_person_status[i].m_move_speed;
    status.g_person_status[i].m_coordinate.second += status.g_person_status[i].m_direction_vector.second * status.g_person_status[i].m_move_speed;
    UpdateDirection(status.g_person_status[i], status.g_boundary);
  }
}

// After a day(move 24 times), check whether infected_person's status needs to be changed.
void RecoveredOrDead(SimulationParameter &status) {
  std::default_random_engine generator(time(NULL));
  std::uniform_real_distribution<float> unif(0, 1);
  // Update recovered or dead.
  for (size_t i = 0; i < status.g_infected_people.size(); ++i) {
    float recovered_prob = unif(generator);
    float dead_prob = unif(generator);
    // recovered
    if (recovered_prob <= status.g_infected_people[i].m_recovery_rate) {
      status.g_person_status[status.g_infected_people[i].m_index].m_status = 2;
      status.g_infected_people[i].m_status = 2;
      --status.g_num_infected;
      ++status.g_num_recovered;
      status.dirty_bit = 1;
    }
    // dead
    else if (dead_prob <= status.g_infected_people[i].m_mortality_rate) {
      status.g_person_status[status.g_infected_people[i].m_index].m_status = 3;
      status.g_infected_people[i].m_status = 3;
      --status.g_num_infected;
      ++status.g_num_dead;
      status.dirty_bit = 1;
    }
  }
  // update infected_person
  for (auto iter = status.g_infected_people.begin(); iter != status.g_infected_people.end();) {
    if (iter->m_status != 1) {
      iter = status.g_infected_people.erase(iter);
    } else {
      ++iter;
    }
  }
}

// check all infected person and spread the virus.
void SpreadVirus(SimulationParameter &status) {
  // Update infected people
  std::default_random_engine generator(time(NULL));
  std::uniform_real_distribution<float> unif(0, 1);
  size_t infect_size = status.g_infected_people.size();
  for(size_t i = 0; i < infect_size; ++i) {
    for(size_t j = 0; j < status.m_total_num_people; ++j) {
      if (status.g_person_status[j].m_status == 0) {
        float dist = sqrt(pow(status.g_infected_people[i].m_coordinate.first - status.g_person_status[j].m_coordinate.first, 2) + pow(status.g_infected_people[i].m_coordinate.second - status.g_person_status[j].m_coordinate.second, 2));
        if (dist <= status.m_spread_range) {
          float infected_prob = unif(generator);
          // been infected, set status = 1 and push it to infected_person.
          if (infected_prob <= status.m_infect_rate) {
            status.g_person_status[j].m_status = 1;
            status.g_infected_people.push_back(status.g_person_status[j]);
            --status.g_num_health;
            ++status.g_num_infected;
            status.dirty_bit = 1;
          }
        }
      }
    }
  }
}


/* Running flow of Simulator
 * 1. Everyone move once.
 * 2. Update everyone's status.
 *
 */
void Run(SimulationParameter &status) {
  size_t count = 100000;
  size_t m_hours = 0;
  while (--count) {
    Move(status);
    SpreadVirus(status);
    ++m_hours;
    // After a day(move 24 times), check whether infected_person's status needs to be changed.
    if (m_hours%24 == 0) {
      m_hours = 0;
      RecoveredOrDead(status);
    }
  }
}

void InitSimulation(SimulationParameter &default_data) {
  // init everyone status
  std::default_random_engine generator(time(NULL));
  std::uniform_real_distribution<float> unif_x(default_data.g_boundary.left_x, default_data.g_boundary.right_x);
  std::uniform_real_distribution<float> unif_y(default_data.g_boundary.down_y, default_data.g_boundary.up_y);
  for (size_t i = 0; i < default_data.m_total_num_people; ++i) {
    default_data.g_person_status[i].m_index = i; // x coord.
    default_data.g_person_status[i].m_coordinate.first = unif_x(generator); // x coord.
    default_data.g_person_status[i].m_coordinate.second = unif_y(generator); // y coord.
    float direct_angle = generator() % 360;
    default_data.g_person_status[i].m_direction_vector.first = cos(direct_angle * M_PI / 180) * default_data.m_move_step; // the move vector of x component
    default_data.g_person_status[i].m_direction_vector.second = sin(direct_angle * M_PI / 180) * default_data.m_move_step; // the move vector of y component
    default_data.g_person_status[i].m_move_speed = default_data.m_move_speed;
    default_data.g_person_status[i].m_status = 0;
    default_data.g_person_status[i].m_recovery_rate = default_data.m_recovery_rate;
    default_data.g_person_status[i].m_mortality_rate = default_data.m_mortality_rate;
  }
  // set init infected people
  for (size_t i = 0; i < default_data.m_infected_people; ++i) {
    default_data.g_person_status[i].m_status = 1;
    default_data.g_infected_people.push_back(default_data.g_person_status[i]);
  }
}

void ClassifyPeople(SimulationParameter &status) {
  status.draw_health.clear();
  status.draw_infected.clear();
  status.draw_recovered.clear();
  status.draw_dead.clear();

  for(size_t i = 0; i < status.m_total_num_people; ++i){
    if(status.g_person_status[i].m_status == 0){
      status.draw_health.push_back(status.g_person_status[i].m_coordinate);
    }
    else if(status.g_person_status[i].m_status == 1){
      status.draw_infected.push_back(status.g_person_status[i].m_coordinate);
    }
    else if(status.g_person_status[i].m_status == 2){
      status.draw_recovered.push_back(status.g_person_status[i].m_coordinate);
    }
    else if(status.g_person_status[i].m_status == 3){
      status.draw_dead.push_back(status.g_person_status[i].m_coordinate);
    }
  }
}

int main() {
  // suppose that the simulate area is 2km * 2km = 4km^2, and the effective range of infection is 2m.
  // everyone move 1m during a time slice.
  SimulationParameter default_data(1000, 50, 1, 1.0, 0.05, 0.0, 100, 0.0, 2.0, 2.0, 0.0, 1000, 1, 0.001, 0.02);
  // init everyone status
  InitSimulation(default_data);

  ShowStatus(default_data, 0);
  Run(default_data);
  ShowStatus(default_data, 0);
  return 0;
}
