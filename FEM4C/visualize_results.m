% visualize_results.m
% Read FEM4C CSV results and produce displacement/stress plots.
%
% Usage:
%   Run this script from the FEM4C project directory (or adjust csvFile
%   below to point at the generated CSV file such as 'output.csv').
%

defaultCandidates = {'output.csv', 'beam_output.csv'};

if ~exist('csvFile', 'var') || strlength(string(csvFile)) == 0
    % Pick the first existing candidate; if none, prompt user.
    csvFile = '';
    for candidate = defaultCandidates
        if isfile(candidate{1})
            csvFile = candidate{1};
            break;
        end
    end
    if strlength(csvFile) == 0
        listing = dir('*.csv');
        if isempty(listing)
            error('No CSV files found in the current directory.');
        end
        csvFile = listing(1).name;
        warning('Using CSV file "%s". Set variable csvFile before running if you want another file.', csvFile);
    end
else
    csvFile = string(csvFile);
end

if ~isfile(csvFile)
    error('CSV file "%s" not found. Please place it in the current directory or update csvFile.', csvFile);
end

fprintf('Loading results from "%s"...\n', csvFile);

% Fall back to manual parsing because detectImportOptions may drop columns for
% irregular rows. We manually split each line to preserve the header structure.
rawLines = splitlines(string(fileread(csvFile)));
rawLines(rawLines == "") = [];
if isempty(rawLines)
    error('CSV file "%s" has no data.', csvFile);
end

headerCells = split(rawLines(1), ',');
numCols = numel(headerCells);

dataCells = strings(numel(rawLines) - 1, numCols);
for i = 2:numel(rawLines)
    parts = split(rawLines(i), ',');
    if numel(parts) < numCols
        parts(numCols) = "";
    end
    dataCells(i - 1, :) = parts(1:numCols).';
end

tbl = array2table(dataCells, 'VariableNames', cellstr(headerCells));
tbl.type = string(tbl.type);
nodeMask = tbl.type == "NODE";
elemMask = tbl.type == "ELEMENT";

nodeTbl = tbl(nodeMask, :);
elemTbl = tbl(elemMask, :);

nodeIDs = str2double(nodeTbl.id);
coords = [str2double(nodeTbl.x), str2double(nodeTbl.y), str2double(nodeTbl.z)];
ux = str2double(nodeTbl.ux);
uy = str2double(nodeTbl.uy);
uz = str2double(nodeTbl.uz);
dispMag = str2double(nodeTbl.disp_mag);

% Sort nodes by ID to ensure consistent indexing.
[nodeIDs, sortIdx] = sort(nodeIDs);
coords = coords(sortIdx, :);
ux = ux(sortIdx);
uy = uy(sortIdx);
uz = uz(sortIdx);
dispMag = dispMag(sortIdx);

elem_n1 = str2double(elemTbl.n1);
elem_n2 = str2double(elemTbl.n2);
elem_n3 = str2double(elemTbl.n3);
validConn = ~(isnan(elem_n1) | isnan(elem_n2) | isnan(elem_n3));

elem_n1 = elem_n1(validConn);
elem_n2 = elem_n2(validConn);
elem_n3 = elem_n3(validConn);
vonElem = str2double(elemTbl.von_mises(validConn));

nodeMap = containers.Map(num2cell(nodeIDs), num2cell(1:numel(nodeIDs)));
faces = zeros(numel(elem_n1), 3);
for e = 1:numel(elem_n1)
    if ~(isKey(nodeMap, elem_n1(e)) && isKey(nodeMap, elem_n2(e)) && isKey(nodeMap, elem_n3(e)))
        error('Element %d references node IDs not present in node list.', e);
    end
    faces(e, :) = [ nodeMap(elem_n1(e)), nodeMap(elem_n2(e)), nodeMap(elem_n3(e)) ];
end

% Average element von Mises stresses to the connected nodes
nodeVon = zeros(size(nodeIDs));
nodeCount = zeros(size(nodeIDs));
for row = 1:size(faces, 1)
    ids = faces(row, :);
    nodeVon(ids) = nodeVon(ids) + vonElem(row);
    nodeCount(ids) = nodeCount(ids) + 1;
end
avgMask = nodeCount > 0;
nodeVon(avgMask) = nodeVon(avgMask) ./ nodeCount(avgMask);

maxMag = max(dispMag);
if maxMag <= 0
    scaleDisp = 1.0;
else
    span = max(coords) - min(coords);
    spanValue = max(span(span > 0));
    if isempty(spanValue) || spanValue == 0
        spanValue = 1.0;
    end
    scaleDisp = 0.1 * spanValue / maxMag;
end
deformedCoords = coords + scaleDisp * [ux, uy, uz];

figure('Name', 'FEM4C Results', 'Color', 'w');
colormap('parula');

% Undeformed configuration
subplot(2, 2, 1);
trisurf(faces, coords(:, 1), coords(:, 2), coords(:, 3), zeros(size(coords, 1), 1), ...
        'EdgeColor', [0.4 0.4 0.4], 'FaceColor', 'interp', 'FaceAlpha', 0.4);
title('Undeformed Mesh');
xlabel('X'); ylabel('Y'); zlabel('Z');
view(2); axis equal tight;

% Deformed displacement magnitude
subplot(2, 2, 2);
trisurf(faces, deformedCoords(:, 1), deformedCoords(:, 2), deformedCoords(:, 3), dispMag, ...
        'EdgeColor', 'k', 'FaceColor', 'interp');
title(sprintf('Deformed Shape (scale = %.2f) with |u|', scaleDisp));
xlabel('X'); ylabel('Y'); zlabel('Z');
view(2); axis equal tight;
colorbar;

% Von Mises stress contour
subplot(2, 2, 3);
trisurf(faces, deformedCoords(:, 1), deformedCoords(:, 2), deformedCoords(:, 3), nodeVon, ...
        'EdgeColor', 'k', 'FaceColor', 'interp');
title('Von Mises Stress (averaged to nodes)');
xlabel('X'); ylabel('Y'); zlabel('Z');
view(2); axis equal tight;
colorbar;

% Quiver plot of in-plane displacements
subplot(2, 2, 4);
scaleFactor = max(vecnorm([ux, uy, uz], 2, 2));
if scaleFactor == 0
    scaleFactor = 1;
end
quiverScale = 0.2 / scaleFactor;
quiver(coords(:, 1), coords(:, 2), ux * quiverScale, uy * quiverScale, 0, 'b');
axis equal tight;
title('In-plane Displacement Vectors (scaled)');
xlabel('X'); ylabel('Y');
grid on;
