#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>
#include <vector>
#include <utility>
#include <mutex>

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
    std::mutex* mutex;
};

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

// std::vector<pos_t> empty_positions, plant_positions, herb_positions;
std::vector<pos_t> already_atualized_pos; //, new_plants, new_herbs, new_carns;
// std::vector<std::pair<pos_t,pos_t>> herb_move, carn_move, plant_eated, herb_eated;

bool random_action(float probability)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    return dis(gen) < probability;
}

pos_t pick_random_cell(std::vector<pos_t> positions)
{
    int rand_index = rand() % positions.size();
    return positions[rand_index];
}

bool check_cell(pos_t pos, std::vector<pos_t> already_atualized_pos)
{
    for (auto &it : already_atualized_pos)
    {
        if (it.i == pos.i && it.j == pos.j)
        {
            return false;
        }
    }
    return true;
}

std::vector<pos_t> check_spec_type(pos_t pos, entity_type_t type){
    pos_t valid_position;
    std::vector<pos_t> val_positions;
    int i = pos.i;
    int j = pos.j;
    if(i + 1 < NUM_ROWS){
        if(check_cell(valid_position, already_atualized_pos)){
            if(entity_grid[i+1][j].type == type){
                valid_position.i = i+1;
                valid_position.j = j;
                val_positions.push_back(valid_position);
            }
        }
    }
    if(j + 1 < NUM_ROWS){
        if(check_cell(valid_position, already_atualized_pos)){
            if(entity_grid[i][j+1].type == type){
                valid_position.i = i;
                valid_position.j = j+1;
                val_positions.push_back(valid_position);
            }
        }
    }
    if(i - 1 >= 0){
        if(check_cell(valid_position, already_atualized_pos)){
            if(entity_grid[i-1][j].type == type){
                valid_position.i = i-1;
                valid_position.j = j;
                val_positions.push_back(valid_position);
            }
        }
    }
    if(j - 1 >= 0){
        if(check_cell(valid_position, already_atualized_pos)){
            if(entity_grid[j][j-1].type == type){
                valid_position.i = i;
                valid_position.j = j-1;
                val_positions.push_back(valid_position);
            }
        }
    }
    return val_positions;
}

void lock_surroundings(pos_t pos){
    entity_grid[pos.i][pos.j].mutex->lock();
    entity_grid[pos.i+1][pos.j].mutex->lock();
    entity_grid[pos.i-1][pos.j].mutex->lock();
    entity_grid[pos.i][pos.j+1].mutex->lock();
    entity_grid[pos.i][pos.j-1].mutex->lock();
}

void unlock_surroundings(pos_t pos){
    entity_grid[pos.i][pos.j].mutex->unlock();
    entity_grid[pos.i+1][pos.j].mutex->unlock();
    entity_grid[pos.i-1][pos.j].mutex->unlock();
    entity_grid[pos.i][pos.j+1].mutex->unlock();
    entity_grid[pos.i][pos.j-1].mutex->unlock();
}

void simulate_plant(pos_t pos){
    std::lock_guard<std::mutex> lock1(*entity_grid[pos.i][pos.j].mutex);
    std::lock_guard<std::mutex> lock2(*entity_grid[pos.i+1][pos.j].mutex);
    std::lock_guard<std::mutex> lock3(*entity_grid[pos.i-1][pos.j].mutex);
    std::lock_guard<std::mutex> lock4(*entity_grid[pos.i][pos.j+1].mutex);
    std::lock_guard<std::mutex> lock5(*entity_grid[pos.i][pos.j-1].mutex);
    if(entity_grid[pos.i][pos.j].age == PLANT_MAXIMUM_AGE){
        entity_grid[pos.i][pos.j].type = empty;
        entity_grid[pos.i][pos.j].age = 0;
    } else if(random_action(PLANT_REPRODUCTION_PROBABILITY)){
        std::vector<pos_t> empty_positions = check_spec_type(pos, empty);
        if(!empty_positions.empty()){
            pos_t chose_position = pick_random_cell(empty_positions);
            entity_grid[chose_position.i][chose_position.j].type = plant;
            entity_grid[chose_position.i][chose_position.j].age = 0;
            already_atualized_pos.push_back(chose_position);
        }
        // Armazena a informação ao invés de atualizar imediatamente a matriz, para evitar que essa informação seja utilizada na mesma iteração
        // new_plants.push_back(chose_position);
        
        // "Reserva" a célula para que não seja usada por outra entidade
        entity_grid[pos.i][pos.j].age++;
        // unlock_surroundings(pos);
    } else {
        entity_grid[pos.i][pos.j].age++;
    }
}

// void simulate_herb(pos_t pos){
//     int i = pos.i;
//     int j = pos.j;
//     if(entity_grid[i][j].age == 50 || entity_grid[i][j].energy == 0){
//         entity_grid[i][j].type = empty;
//         entity_grid[i][j].age = 0;
//         entity_grid[i][j].energy = 0;
//     } else if(random_action(HERBIVORE_REPRODUCTION_PROBABILITY) && 
//                 entity_grid[i][j].energy > THRESHOLD_ENERGY_FOR_REPRODUCTION &&
//                 !empty_positions.empty()){
//             entity_grid[i][j].energy = entity_grid[i][j].energy - 10;
//             pos_t chose_position = pick_random_cell(empty_positions);
//             new_herbs.push_back(chose_position);
//             already_atualized_pos.push_back(chose_position);
//     } else if(random_action(HERBIVORE_EAT_PROBABILITY) && !plant_positions.empty()){
//             pos_t chose_position = pick_random_cell(plant_positions);
//             plant_eated.push_back(std::make_pair(pos, chose_position));
//             already_atualized_pos.push_back(chose_position);
//     } else if(random_action(HERBIVORE_MOVE_PROBABILITY) && !empty_positions.empty()){
//             pos_t chose_position = pick_random_cell(empty_positions);
//             herb_move.push_back(std::make_pair(pos, chose_position));
//             already_atualized_pos.push_back(chose_position);
//     } else entity_grid[i][j].age++;
// }

// void simulate_carn(pos_t pos){
//     int i = pos.i;
//     int j = pos.j;
//     if(entity_grid[i][j].age == 80 || entity_grid[i][j].energy == 0){
//         entity_grid[i][j].type = empty;
//         entity_grid[i][j].age = 0;
//         entity_grid[i][j].energy = 0;
//     } else if(random_action(CARNIVORE_REPRODUCTION_PROBABILITY) && 
//                 entity_grid[i][j].energy > THRESHOLD_ENERGY_FOR_REPRODUCTION &&
//                 !empty_positions.empty()){
//             entity_grid[i][j].energy = entity_grid[i][j].energy - 10;
//             pos_t chose_position = pick_random_cell(empty_positions);
//             new_carns.push_back(chose_position);
//             already_atualized_pos.push_back(chose_position);
//     } else if(random_action(CARNIVORE_EAT_PROBABILITY) && !herb_positions.empty()){
//             pos_t chose_position = pick_random_cell(herb_positions);
//             herb_eated.push_back(std::make_pair(pos, chose_position));
//             already_atualized_pos.push_back(chose_position);
//     } else if(random_action(CARNIVORE_MOVE_PROBABILITY) && !empty_positions.empty()){
//             pos_t chose_position = pick_random_cell(empty_positions);
//             carn_move.push_back(std::make_pair(pos, chose_position));
//             already_atualized_pos.push_back(chose_position);
//     } else entity_grid[i][j].age++;
// }

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}


int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, {empty, 0, 0, new std::mutex()}));
        
        // Create the entities
        int i;
        int row, col;
        for(i = 0; i < (uint32_t)request_body["plants"]; i++){
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 14);
            row = dis(gen);
            col = dis(gen);

            while(!entity_grid[row][col].type == empty){
                row = dis(gen);
                col = dis(gen);
            }
            
            entity_grid[row][col].type = plant;
            entity_grid[row][col].age = 0; 
        }
        for(i = 0; i < (uint32_t)request_body["herbivores"]; i++){
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 14);
            row = dis(gen);
            col = dis(gen);

            while(!entity_grid[row][col].type == empty){
                row = dis(gen);
                col = dis(gen);
            }
            
            entity_grid[row][col].type = herbivore;
            entity_grid[row][col].age = 0;
            entity_grid[row][col].energy = 100; 
        }
        for(i = 0; i < (uint32_t)request_body["carnivores"]; i++){
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 14);
            row = dis(gen);
            col = dis(gen);

            while(!entity_grid[row][col].type == empty){
                row = dis(gen);
                col = dis(gen);
            }
            
            entity_grid[row][col].type = carnivore;
            entity_grid[row][col].age = 0;
            entity_grid[row][col].energy = 100;
        }

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behaviour of each entity
        
        int i, j;
        pos_t valid_position;
        pos_t chose_position;
        pos_t current_pos;
        std::vector<std::thread> active_threads;

        for (i = 0; i < NUM_ROWS; i++){
            for (j = 0; j < NUM_ROWS; j++){
                current_pos.i = i;
                current_pos.j = j;
                if(check_cell(current_pos, already_atualized_pos)){
                    if(entity_grid[i][j].type != empty){
                        if(entity_grid[i][j].type == plant){
                            std::thread t_plant(simulate_plant,current_pos);
                            active_threads.push_back(t_plant);
                        } else if(entity_grid[i][j].type == herbivore){
                            // std::thread t_herb(simulate_herb,current_pos);
                            // t_herb.join();
                        } else if(entity_grid[i][j].type == carnivore){
                            // std::thread t_carn(simulate_carn,current_pos);
                            // t_carn.join();
                        }
                    }
                }
            }
        }
        for(auto& it : active_threads){
            it.join();
        }
        already_atualized_pos.clear();
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}