from flask import Flask, request, jsonify
import copy

app = Flask(__name__)

# Define directions for moving in the matrix
dx = [-1, 0, 1, 0]  # Up, Right, Down, Left
dy = [0, 1, 0, -1]

def neighbors(pos, maze):
    x, y = pos
    result = []
    for direction in range(4):
        nx, ny = x + dx[direction], y + dy[direction]
        if 0 <= nx < len(maze) and 0 <= ny < len(maze[0]) and maze[nx][ny] != '*':
            result.append((nx, ny))
    return result

def dfs(maze, start, goal):
    stack = [(start, [start])]
    visited = set()
    visited.add(start)
    
    while stack:
        (vertex, path) = stack.pop()
        for next in neighbors(vertex, maze):
            if next not in visited:
                if next == goal:
                    return path + [next]
                else:
                    stack.append((next, path + [next]))
                    visited.add(next)
    return []

@app.route('/update_maze', methods=['POST'])
def update_maze():
    data = request.get_json()
    matrix = data['matrix']
    current = tuple(data['current'])
    goal = tuple(data['goal'])
    
    path = dfs(matrix, current, goal)
    
    instructions = []
    if path:
        current_pos = current
        for pos in path[1:]:
            if pos[0] == current_pos[0] - 1:
                instructions.append("MOVE_UP")
            elif pos[0] == current_pos[0] + 1:
                instructions.append("MOVE_DOWN")
            elif pos[1] == current_pos[1] - 1:
                instructions.append("MOVE_LEFT")
            elif pos[1] == current_pos[1] + 1:
                instructions.append("MOVE_RIGHT")
            current_pos = pos

    # Only return the next move
    next_instruction = [instructions[0]] if instructions else []
    
    return jsonify({
        "matrix": matrix,
        "instructions": next_instruction,
        "path": path
    })

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
