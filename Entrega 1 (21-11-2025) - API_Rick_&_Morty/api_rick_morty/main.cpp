#include <iostream>
#include <string>
#include <vector>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Character
{
    std::string name;
    std::string origin;
    std::string species;
    std::string status;
    std::vector<std::string> episodes;
};


std::string obtenerNombres(const std::string &url)
{
    auto response = cpr::Get(
        cpr::Url{url});

    if (response.error)
    {
        std::cerr << "Error en la petición HTTP: " << response.error.message << "\n";
        return "Desconocido";
    }

    if (response.status_code != 200)
    {
        std::cerr << "La API devolvió código HTTP "
                  << response.status_code << "\nCuerpo:\n"
                  << response.text << "\n";
        return "Desconocido";
    }
    json j = json::parse(response.text);
    return j.value("name","Desconocido");
}

std::vector<Character> parse_characters(const std::string &body)
{
    std::vector<Character> characters;

    json j = json::parse(body);

    if (!j.contains("results") || !j["results"].is_array())
    {
        return characters;
    }

    for (const auto &item : j["results"])
    {
        Character c;
        std :: vector <std::string> urls {};

        c.name = item.value("name", "Desconocido");
        c.status = item.value("status", "Desconocido");
        c.species = item.value("species", "Desconocida");

        if (item.contains("origin") && item["origin"].is_object())
        {
            c.origin = item["origin"].value("name", "Desconocido");
        }
        else
        {
            c.origin = "Desconocido";
        }

        if(item.contains("episode") && item["episode"].is_array())
        {
            for(const auto &url : item["episode"])
            {
                c.episodes.push_back(std::move(url));
            }
        }

        characters.push_back(std::move(c));
    }

    return characters;
}

int main()
{
    std::cout << "Introduce el nombre (o parte del nombre) del personaje de Rick & Morty: ";
    std::string search;
    std::getline(std::cin, search);

    if (search.empty())
    {
        std::cerr << "Cadena de búsqueda vacía. Terminando.\n";
        return 1;
    }

    // Petición GET con cpr, usando parámetros (hace URL encoding por ti)
    auto response = cpr::Get(
        cpr::Url{"https://rickandmortyapi.com/api/character/"},
        cpr::Parameters{{"name", search}});

    if (response.error)
    {
        std::cerr << "Error en la petición HTTP: " << response.error.message << "\n";
        return 1;
    }

    if (response.status_code != 200)
    {
        std::cerr << "La API devolvió código HTTP "
                  << response.status_code << "\nCuerpo:\n"
                  << response.text << "\n";
        return 1;
    }

    auto characters = parse_characters(response.text);
    if (characters.empty())
    {
        std::cout << "No se encontraron resultados para: " << search << "\n";
        return 0;
    }

    // Mostrar listado de resultados
    std::cout << "\nResultados encontrados:\n";
    for (std::size_t i{0}; i < characters.size(); ++i)
    {
        std::cout << i << ") " << characters.at(i).name << '\n';
    }

    // Seleccionar uno
    std::cout << "\nSelecciona el índice del personaje que te interesa: ";
    std::size_t index = 0;
    if (!(std::cin >> index))
    {
        std::cerr << "Entrada no válida.\n";
        return 1;
    }

    if (index >= characters.size())
    {
        std::cerr << "Índice fuera de rango.\n";
        return 1;
    }

    const auto &c = characters.at(index);

    std::cout << "\n--- Detalles del personaje ---\n";
    std::cout << "Nombre : " << c.name << '\n';
    std::cout << "Planeta (origen): " << c.origin << '\n';
    std::cout << "Especie: " << c.species << '\n';
    std::cout << "Status : " << c.status << '\n';

    // Mostrar los episodios de cada personaje
    for(auto const &ep : c.episodes)
    {
        std::string name = obtenerNombres(ep);
        std :: cout << "Episodio: " << name << std::endl;
    }

    return 0;
}
