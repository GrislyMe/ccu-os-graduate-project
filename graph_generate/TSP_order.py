#!/usr/bin/env python3
from ortools.constraint_solver import routing_enums_pb2
from ortools.constraint_solver import pywrapcp
import numpy as np


def create_data_model(time: np.ndarray, num_of_vechicles: int):
    data = {}
    data['distance_matrix'] = time.tolist()
    data['num_of_vehicles'] = num_of_vechicles
    data['depot'] = 0
    return data


def result(manager, routing, solution):
    index = routing.Start(0)
    plan_output = 'Route: '
    while not routing.IsEnd(index):
        plan_output += f"{manager.IndexToNode(index)}, "
        index = solution.Value(routing.NextVar(index))
    plan_output += f"{manager.IndexToNode(index)}\n"
    return plan_output


def main(time: np.ndarray, num_of_vechicles: int):
    data = create_data_model(time, num_of_vechicles)

    manager = pywrapcp.RoutingIndexManager(len(data['distance_matrix']),
            data['num_of_vehicles'], data['depot'])

    routing = pywrapcp.RoutingModel(manager)

    def distance_callback(from_index, to_index):
        from_node = manager.IndexToNode(from_index)
        to_node = manager.IndexToNode(to_index)
        return data['distance_matrix'][from_node][to_node]

    transit_callback_index = routing.RegisterTransitCallback(distance_callback)

    routing.SetArcCostEvaluatorOfAllVehicles(transit_callback_index)

    search_parameters = pywrapcp.DefaultRoutingSearchParameters()
    search_parameters.first_solution_strategy = (
        routing_enums_pb2.FirstSolutionStrategy.PATH_CHEAPEST_ARC)

    solution = routing.SolveWithParameters(search_parameters)

    if solution:
        return result(manager, routing, solution)
    return None
