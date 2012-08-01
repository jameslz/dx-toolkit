'''
This file contains all the utilities needed for escaping and parsing
names in the syntax of

    project-ID-or-name:folder/path/to/filename

For more details, see external documentation [TODO: Put link here].
'''

import dxpy
import re, os, sys
from dxpy.utils.describe import *

def pick(choices, default=None, str_choices=None, prompt=None, allow_mult=False):
    '''
    :param choices: Strings between which the user will make a choice
    :type choices: list of strings
    :param default: Number the index to be used as the default
    :type default: int or None
    :param str_choices: Strings to be used as aliases for the choices; must be of the same length as choices and each string must be unique
    :type str_choices: list of strings
    :param prompt: A custom prompt to be used
    :type prompt: string
    :returns: The user's choice as a numbered index of choices (e.g. 0 for the first item)
    :rtype: int
    :raises: :exc:`EOFError` to signify quitting the process
    '''
    for i in range(len(choices)):
        print str(i) + ') ' + choices[i]
    print ''
    if prompt is None:
        if default is not None:
            prompt = 'Pick a numbered choice [' + str(default) + ']: '
        else:
            prompt = 'Pick a numbered choice: '
    while True:
        try: 
            value = raw_input(prompt)
        except KeyboardInterrupt:
            print ''
            continue
        except EOFError:
            print ''
            raise EOFError()
        if default is not None and value == '':
            return default
        if allow_mult and value == '*':
            return value
        try:
            choice = str_choices.index(value)
            return choice
        except:
            pass
        try:
            choice = int(value)
            if choice not in range(len(choices)):
                raise IndexError()
            return choice
        except BaseException as details:
            print str(details)
            print 'Not a valid selection'

# The following caches project names to project IDs because they are
# unlikely to change.
cached_project_names = {}
# Possible cache for the future of project ID->folderpath->object name->ID
# cached_project_paths = {}

class ResolutionError(Exception):
    def __init__(self, msg):
        self.msg = msg

    def __str__(self):
        return self.msg

data_obj_pattern = re.compile('^(record|table|gtable|program|file)-[0-9A-Za-z]{24}$')
hash_pattern = re.compile('^(record|table|gtable|app|program|job|project|workspace|container|file)-[0-9A-Za-z]{24}$')
nohash_pattern = re.compile('^(user|org|app|team)-')

def is_hashid(string):
    return hash_pattern.match(string) is not None

def is_data_obj_id(string):
    return data_obj_pattern.match(string) is not None

def is_container_id(string):
    return is_hashid(string) and (string.startswith('project-') or string.startswith('workspace-') or string.startswith('container-'))

def is_nohash_id(string):
    return nohash_pattern.match(string) is not None

def escape_folder_str(string):
    return string.replace('\\', '\\\\').replace(' ', '\ ').replace(':', '\:')

def escape_name_str(string):
    return string.replace('\\', '\\\\').replace(' ', '\ ').replace(':', '\:').replace('/', '\/')

def unescape_folder_str(string):
    return string.replace('\:', ':').replace('\ ', ' ').replace('\\\\', '\\')

def unescape_name_str(string):
    return string.replace('\:', ':').replace('\ ', ' ').replace('\/', '/').replace('\\\\', '\\')

def get_last_pos_of_char(char, string):
    '''
    :param char: The character to find
    :type char: string
    :param string: The string in which to search for *char*
    :type string: string
    :returns: Index in *string* where *char* last appears (unescaped by a preceding "\\"), -1 if not found
    :rtype: int

    Finds the last occurrence of *char* in *string* in which *char* is
    not present as an escaped character.

    '''
    pos = len(string)
    while pos > 0:
        pos = string[:pos].rfind(char)
        if pos == -1:
            return -1
        num_backslashes = 0
        test_index = pos - 1
        while test_index >= 0 and string[test_index] == '\\':
            num_backslashes += 1
            test_index -= 1
        if num_backslashes % 2 == 0:
            return pos
    return -1

def split_unescaped(char, string):
    '''
    :param char: The character on which to split the string
    :type char: string
    :param string: The string to split
    :type string: string
    :returns: List of substrings of *string*
    :rtype: list of strings

    Splits *string* whenever *char* appears without an odd number of
    backslashes ('\\') preceding it, discarding any empty string
    elements.

    '''
    words = []
    pos = len(string)
    lastpos = pos
    while pos >= 0:
        pos = get_last_pos_of_char(char, string[:lastpos])
        if pos >= 0:
            if pos + 1 != lastpos:
                words.append(string[pos + 1: lastpos])
            lastpos = pos
    if lastpos != 0:
        words.append(string[:lastpos])
    words.reverse()
    return words

def clean_folder_path(path, expected=None):
    '''
    :param path: A folder path to sanitize and parse
    :type path: string
    :param expected: Whether a folder ("folder"), a data object ("entity"), or either (None) is expected
    :type expected: string or None
    :returns: *folderpath*, *name*

    Unescape and parse *path* as a folder path to possibly an entity
    name.  Consecutive unescaped forward slashes "/" are collapsed to
    a single forward slash.  If *expected* is "folder", *name* is
    always returned as None.  Otherwise, the string to the right of
    the last unescaped "/" is considered a possible data object name
    and returned as such.

    '''
    folders = split_unescaped('/', path)

    if len(folders) == 0:
        return '/', None

    if expected == 'folder' or folders[-1] == '.' or folders[-1] == '..' or get_last_pos_of_char('/', path) == len(path) - 1:
        entity_name = None
    else:
        entity_name = unescape_name_str(folders[-1])
        folders = folders[:-1]

    sanitized_folders = []

    for folder in folders:
        if folder == '.':
            pass
        elif folder == '..':
            if len(sanitized_folders) > 0:
                sanitized_folders.pop()
        else:
            sanitized_folders.append(unescape_folder_str(folder))

    if len(sanitized_folders) == 0:
        newpath = '/'
    else:
        newpath = ""
        for folder in sanitized_folders:
            newpath += '/' + folder

    return newpath, entity_name

def resolve_container_id_or_name(raw_string, is_error=False, unescape=True, multi=False):
    '''
    :param raw_string: A potential project or container ID or name
    :type raw_string: string
    :param is_error: Whether to raise an exception if the project or container ID cannot be resolved
    :type is_error: boolean
    :param unescape: Whether to unescaping the string is required (TODO: External link to section on escaping characters.)
    :type unescape: boolean
    :returns: Project or container ID if found or else None
    :rtype: string or None
    :raises: :exc:`ResolutionError` if *is_error* is True and the project or container could not be resolved

    Attempt to resolve *raw_string* to a project or container ID.

    '''
    if unescape:
        string = unescape_name_str(raw_string)
    if is_container_id(string):
        return ([string] if multi else string)

    if string in cached_project_names:
        return ([cached_project_names[string]] if multi else cached_project_names[string])

    try:
        results = list(dxpy.find_projects(name=string, describe=True, level='LIST'))
    except BaseException as details:
        raise ResolutionError(str(details))

    if len(results) == 1:
        cached_project_names[string] = results[0]['id']
        return ([results[0]['id']] if multi else results[0]['id'])
    elif len(results) == 0:
        if is_error:
            raise ResolutionError('Could not resolve container ID or name')
        return ([] if multi else None)
    elif not multi:
        print 'Found multiple projects with name \"' + string + '\"'
        choice = pick(map(lambda result: result['id'] + ' (' + result['level'] + ')', results))
        return results[choice]['id']
    else:
        # len(results) > 1 and multi
        return map(lambda result: result['id'], results)

def resolve_path_with_project(path, expected=None, expected_classes=None, multi_projects=False):
    '''
    :param path: A path to a data object to attempt to resolve
    :type path: string
    :param expected: one of the following: "folder", "entity", or None to indicate whether the expected path is a folder, a data object, or either
    :type expected: string or None
    :type expected_classes: a list of DNAnexus data object classes (if any) by which the search can be filtered
    :type expected_classes: list of strings or None
    :returns: A tuple of 3 values: container_ID, folderpath, entity_name
    :rtype: string, string, string
    :raises: exc:`ResolutionError` if 1) a colon is provided but no project can be resolved, or 2) *expected* was set to "folder" but no project can be resolved from which to establish context

    Attempts to resolve *path* to a project or container ID, a folder
    path, and a data object or folder name.  This method will NOT
    raise an exception if the specified folder or object does not
    exist.  This method is primarily for parsing purposes.

    '''

    # Easy case: ":"
    if path == ':':
        if dxpy.WORKSPACE_ID is None:
            raise ResolutionError('Expected a project name or ID to the left of a colon or for a current project to be set.')
        return ([dxpy.WORKSPACE_ID] if multi_projects else dxpy.WORKSPACE_ID), '/', None
    # Second easy case: empty string
    if path == '':
        if dxpy.WORKSPACE_ID is None:
            raise ResolutionError('Expected a project name or ID to the left of a colon or for a current project to be set.')
        return ([dxpy.WORKSPACE_ID] if multi_projects else dxpy.WORKSPACE_ID), os.environ.get('DX_CLI_WD', '/'), None
    # Third easy case: hash ID
    if is_container_id(path):
        return ([path] if multi_projects else path), '/', None
    elif is_hashid(path):
        return ([dxpy.WORKSPACE_ID] if multi_projects else dxpy.WORKSPACE_ID), None, path

    project = None
    folderpath = None
    entity_name = None
    wd = None

    # Test for multiple colons
    last_colon = get_last_pos_of_char(':', path)
    if last_colon >= 0:
        last_last_colon = get_last_pos_of_char(':', path[:last_colon])
        if last_last_colon >= 0:
            raise ResolutionError('At most one colon expected in a path')

    substrings = split_unescaped(':', path)

    if len(substrings) == 2:
        # project-name-or-id:folderpath/to/possible/entity
        if multi_projects:
            project_ids = resolve_container_id_or_name(substrings[0], is_error=True, multi=True)
        else:
            project = resolve_container_id_or_name(substrings[0], is_error=True)
        wd = '/'
    elif get_last_pos_of_char(':', path) >= 0:
        # :folderpath/to/possible/entity OR project-name-or-id:
        # Colon is either at the beginning or at the end
        wd = '/'
        if path.startswith(':'):
            if dxpy.WORKSPACE_ID is None:
                raise ResolutionError('Expected a project name or ID to the left of a colon or for a current project to be set')
            project = dxpy.WORKSPACE_ID
        else:
            # One nonempty string to the left of a colon
            project = resolve_container_id_or_name(substrings[0], is_error=True)
            folderpath = '/'
    else:
        # One nonempty string, no colon present, do NOT interpret as
        # project
        project = dxpy.WORKSPACE_ID
        if expected == 'folder' and project is None:
            raise ResolutionError('Could not resolve a project name or ID')
        wd = os.environ.get('DX_CLI_WD', '/')

    # Determine folderpath and entity_name if necessary
    if folderpath is None:
        folderpath = substrings[-1]
        folderpath, entity_name = clean_folder_path(('' if len(folderpath) > 0 and folderpath[0] == '/' else wd + '/') + folderpath, expected)

    if multi_projects:
        return (project_ids if project is None else [project]), folderpath, entity_name
    else:
        return project, folderpath, entity_name

def resolve_existing_path(path, expected=None, ask_to_resolve=True, expected_classes=None, allow_mult=False, describe={}):
    '''
    :param ask_to_resolve: Whether picking may be necessary
    :type ask_to_resolve: boolean
    :param allow_mult: Whether to allow the user to select multiple results from the same path
    :type allow_mult: boolean
    :returns: A LIST of results when ask_to_resolve is False or allow_mult is True

    Returns either a list of results or a single result (depending on
    how many is expected; if only one, then an interactive picking of
    a choice will be initiated if input is a tty, or else throw an error).

    Output is of the form {"id": id, "describe": describe hash} a list
    of those

    TODO: Allow arbitrary flags for the describe hash.

    NOTE: if expected_classes is provided and conflicts with the class
    of the hash ID, it will return None for all fields.
    '''

    project, folderpath, entity_name = resolve_path_with_project(path, expected)
    if entity_name is None:
        # Definitely a folder (or project)
        # FIXME? Should I check that the folder exists if expected="folder"?
        return project, folderpath, entity_name
    elif is_hashid(entity_name):
        found_valid_class = True
        if expected_classes is not None:
            found_valid_class = False
            for klass in expected_classes:
                if entity_name.startswith(klass):
                    found_valid_class = True
        if not found_valid_class:
            return None, None, None
        try:
            desc = dxpy.DXHTTPRequest('/' + entity_name + '/describe', {})
        except BaseException as details:
            raise ResolutionError(str(details))
        result = {"id": entity_name, "describe": desc}
        if ask_to_resolve and not allow_mult:
            return project, folderpath, result
        else:
            return project, folderpath, [result]
    else:
        msg = 'Object of name ' + entity_name + ' could not be resolved in folder ' + folderpath + ' of project ID ' + project
        # Probably an object
        results = list(dxpy.find_data_objects(project=project,
                                              folder=folderpath,
                                              name=entity_name,
                                              recurse=False,
                                              describe=describe,
                                              visibility='either'))
        if len(results) == 0:
            # Could not find it as a data object.  If anything, it's a
            # folder.

            if '/' in entity_name:
                # Then there's no way it's supposed to be a folder
                raise ResolutionError(msg)

            # This is the only possibility left.  Leave the
            # error-checking for later.  Note that folderpath does
            possible_folder = folderpath + '/' + entity_name
            possible_folder, skip = clean_folder_path(possible_folder, 'folder')
            return project, possible_folder, None

        # Caller wants ALL results; just return the whole thing
        if not ask_to_resolve:
            return project, None, results

        if len(results) > 1:
            if sys.stdout.isatty():
                print 'The given path \"' + path + '\" resolves to the following data objects:'
                choice = pick(map(lambda result:
                                      get_ls_l_desc(result['describe']),
                                  results),
                              allow_mult=allow_mult)
                if allow_mult and choice == '*':
                    return project, None, results
                else:
                    return project, None, ([results[choice]] if allow_mult else results[choice])
            else:
                raise ResolutionError('The given path \"' + path + '\" resolves to ' + str(len(results)) + ' data objects')
        elif len(results) == 1:
            return project, None, ([results[0]] if allow_mult else results[0])

def get_app_from_path(path):
    '''
    :param path: A string to attempt to resolve to an app object
    :type path: string
    :returns: The describe hash of the app object if found, or None otherwise
    :rtype: dict or None

    This method parses a string that is expected to perhaps refer to
    an app object.  If found, its describe hash will be returned.  For
    more information on the contents of this hash, see the API
    documentation. [TODO: external link here]

    '''
    alias = None
    if not path.startswith('app-'):
        path = 'app-' + path
    if '/' in path:
        alias = path[path.find('/') + 1:]
        path = path[:path.find('/')]
    try:
        if alias is None:
            desc = dxpy.DXHTTPRequest('/' + path + '/describe', {})
        else:
            desc = dxpy.DXHTTPRequest('/' + path + '/' + alias + '/describe', {})
        return desc
    except dxpy.DXAPIError as details:
        return None
