from ctypes import byref, c_int, c_char, c_char_p, Structure, CDLL, POINTER
import enum


LIB_EUPD_OK = 0
LIB_EUPD_W_LIST_INCOMPLETE = 0x100
LIB_EUPD_W_NOT_FOUND = 0x101
LIB_EUPD_E_NO_MEMORY = 0x200
LIB_EUPD_E_CURL_SETUP = 0x201
LIB_EUPD_E_CANNOT_RESOLVE = 0x202
LIB_EUPD_E_CONNECTION_FAILED = 0x203
LIB_EUPD_E_HTTP_ERROR = 0x203
LIB_EUPD_E_TRANSFER_ERROR = 0x204
LIB_EUPD_E_TIMEOUT = 0x205
LIB_EUPD_E_SSL = 0x206
LIB_EUPD_E_UNKW_NETWORK = 0x207
LIB_EUPD_E_MALFORMED_LIST = 0x208
LIB_EUPD_E_INVALID_ARGUMENT = 0x209

LIB_EUST_UNKWOWN = 0
LIB_EUST_UP_TO_DATE = 1
LIB_EUST_AVAILABLE = 2
LIB_EUST_RECOMMENDED = 3
LIB_EUST_REQUIRED = 4


class LIB_VERSION(Structure):
    _fields_ = [('major', c_int),
                ('minor', c_int),
                ('revision', c_char * 4)]


class LIB_INSOFTWARE(Structure):
    _fields_ = [('name', c_char * 32),
                ('version', LIB_VERSION)]


class LIB_RESULT(Structure):
    _fields_ = [('status', c_int),
                ('version', LIB_VERSION),
                ('link', c_char_p)]


class ECHMETUpdateCheck:
    """Wrapper around `libECHMETUpdateCheck` library
    """

    class ErrorType(enum.Enum):
        NO_ERROR = 1,
        NETWORK_ERROR = 2
        PROCESSING_ERROR = 3

    class UpdateState(enum.Enum):
        UP_TO_DATE = 1
        UPDATE_AVAILABLE = 2
        UPDATE_RECOMMENDED = 3
        UPDATE_REQUIRED = 4

    class InvalidInputError(Exception):
        def __init__(self, msg):
            super().__init__()
            self.msg = msg

        def __str__(self):
            return self.msg

    class InvalidOutputError(Exception):
        def __init__(self, msg):
            super().__init__()
            self.msg = msg

        def __str__(self):
            return self.msg

    class Version:
        """Software version descriptor.

        Attributes:
            major (int): Major version number.
            minor (int): Minor version number.
            revision (str): Revision string.
        """

        def __init__(self, major, minor, revision):
            """:obj:Version constructor.

            Args:
                major (int): Major version number.
                minor (int): Minor version number.
                revision (bytes): Revision string.

            Raises:
                InvalidInputError: Version description is malformed.
            """

            self.major = c_int(major)
            self.minor = c_int(minor)

            asc = revision.encode('ASCII')
            if len(asc) > 4:
                raise ECHMETUpdateCheck.InvalidInputError('Revision string is '
                                                          'too long')
            self.revision = asc

        def __str__(self):
            return 'Major: {0}, Minor: {1}, Revision: {2}'.format(
                    self.major.value,
                    self.minor.value,
                    self.revision.decode('ASCII'))

    class Software:
        """Software descriptor.

        Attributes:
            name (bytes): Name of the software.
            version (:obj:Version): Version of the software.
        """

        def __init__(self, name, version):
            """:obj:Software constructor.

            Args:
                nane (str): Name of the software.
                version (:obj:Version): Version of the software.

            Raises:
                InvalidInputError: Software description is malformed
            """

            asc = name.encode('ASCII')
            if len(asc) > 32:
                raise ECHMETUpdateCheck.InvalidInputError('Name string is '
                                                          'too long')
            self.name = asc

            self.version = version

    class Result:
        """Update check result descriptor.

        Attributes:
            status (:obj:UpdateStatus): Result of update check.
            version (:obj:Version): Latest available version of the software.
            link (str): Download link for the latest available version.
        """

        def __init__(self, status, version, link):
            self.status = status
            self.version = version
            self.link = link

        def __str__(self):
            return ('Status: {0},\n'
                    'Latest version: {1}\n'
                    'Download link: {2}').format(
                        self.status, self.version, self.link)

    def __init__(self, path):
        """:obj:ECHMETUpdateCheck constructor.

        Args:
            path (str): Path to the `libECHMETUpdateCheck` library.

        Raises:
            AttributeError: Required symbol was not found in the library.
        """

        self.lib_obj = CDLL(path)

        self.lib_obj.updater_error_to_str.restype = c_char_p
        self.lib_obj.updater_status_to_str.restype = c_char_p

        if not hasattr(self.lib_obj, 'updater_check'):
            raise AttributeError('undefined symbol: updater_check')

        if not hasattr(self.lib_obj, 'updater_check_many'):
            raise AttributeError('undefined symbol: updater_check_many')

        if not hasattr(self.lib_obj, 'updater_free_result'):
            raise AttributeError('undefined symbol: updater_free_result')

        if not hasattr(self.lib_obj, 'updater_free_result_list'):
            raise AttributeError('undefined symbol: updater_free_result_list')

    @staticmethod
    def internal_is_error(ret):
        """Checks whether `ECHMETUpdateCheck` return code represents
        an error state.

        Args:
            ret (int): `ECHMETUpdateCheck` return code.

        Returns:
            True if the code is an error, False otherwise.
        """

        return ret >= 0x200

    @staticmethod
    def internal_is_network_error(ret):
        """Checks whether `ECHMETUpdateCheck` return code represents
        a network error state.

        Args:
            ret (int): `ECHMETUpdateCheck` return code

        Returns:
            True if the code is a network error, False otherwise.
        """

        return ret >= 0x300 and ret < 0x400

    @staticmethod
    def internal_is_warning(ret):
        """Checks whether `ECHMETUpdateCheck` return code represents
        a warning state.

        Args:
            ret (int): `ECHMETUpdateCheck` return code.

        Returns:
            True if the code is a warning, False otherwise.
        """

        return ret >= 0x100 and ret < 0x200

    @staticmethod
    def error_type(ret):
        """Converts library return code to an error class. Warning is
        not considered an error.

        Args:
            ret (int): `ECHMETUpdateCheck` return code.

        Returns:
            :obj:ErrorType of the corresponding error type.
        """

        if ECHMETUpdateCheck.internal_is_network_error(ret):
            return ECHMETUpdateCheck.ErrorType.NETWORK_ERROR
        elif ECHMETUpdateCheck.internal_is_error(ret):
            return ECHMETUpdateCheck.ErrorType.PROCESSING_ERROR
        return ECHMETUpdateCheck.ErrorType.NO_ERROR

    def lib_to_usr_status(self, res):
        if res == LIB_EUST_UP_TO_DATE:
            return self.UpdateState.UP_TO_DATE
        if res == LIB_EUST_AVAILABLE:
            return self.UpdateState.UPDATE_AVAILABLE
        if res == LIB_EUST_RECOMMENDED:
            return self.UpdateState.UPDATE_RECOMMENDED
        if res == LIB_EUST_REQUIRED:
            return self.UpdateState.UPDATE_REQUIRED

        raise ECHMETUpdateCheck.InvalidOutputError('Unknown value of '
                                                   'update status')

    def check(self, url, software, allow_insecure):
        """Checks if there is an update available for given software.

        Args:
            url (str): URL of the list of updates.
            software (:obj:Software): Software to check.
            allow_insecure (bool): Allow HTTP and ignore TLS errors. This is
                                   dangerous and shall not be used
                                   in production.

        Returns:
            Tuple with the following fields:
            [0] (bool): ``True`` if the check succeeded, ``False`` otherwise.
            [1] (int): ``ECHMETUpdateCheck`` return code.
            [2] (:obj:Result) The actual result. If the check failed,
                the value is set to ``None``
        """

        in_sw = LIB_INSOFTWARE()
        in_sw.name = software.name
        in_sw.version.major = software.version.major
        in_sw.version.minor = software.version.minor
        in_sw.version.revision = software.version.revision

        res = LIB_RESULT()

        ret = self.lib_obj.updater_check(url.encode('ASCII'), byref(in_sw),
                                         byref(res), c_int(allow_insecure))
        if self.internal_is_error(ret):
            return (False, ret, None)

        if res.status == LIB_EUST_UNKWOWN:
            return (False, ret, None)

        usr_res = self.Result(self.lib_to_usr_status(res.status),
                              self.Version(res.version.major,
                                           res.version.minor,
                                           res.version.revision
                                           .decode('ASCII')),
                              res.link.decode('UTF-8'))

        self.lib_obj.updater_free_result(byref(res))

        return (True, ret, usr_res)

    def check_many(self, url, software_list, allow_insecure):
        """Checks if there are updates available for multiple softwares

        Args:
            url (str): URL of the list of updates.
            software_list (:obj:Software): Array of softwares to check.
            allow_insecure (bool): Allow HTTP and ignore TLS errors. This is
                                   dangerous and shall not be used
                                   in production.

        Returns:
            Tuple with the following fields:
            [0] (bool): ``True`` if the check succeeded, ``False`` otherwise.
            [1] (int): ``ECHMETUpdateCheck`` return code.
            [2] Array of tuples with the following fields:
                [0] (str): Name of software
                [1] (:obj:Result): Update check result for the given software

        """

        num_software = len(software_list)

        INSWLIST_TYPE = LIB_INSOFTWARE * num_software
        RESULTS_TYPE = POINTER(LIB_RESULT)
        in_sw_list = INSWLIST_TYPE()

        for idx in range(0, num_software):
            in_sw = in_sw_list[idx]
            in_sw.name = software_list[idx].name
            in_sw.version.major = software_list[idx].version.major
            in_sw.version.minor = software_list[idx].version.minor
            in_sw.version.revision = software_list[idx].version.revision

        results = RESULTS_TYPE()
        num_results = c_int(0)

        ret = self.lib_obj.updater_check_many(url.encode('ASCII'),
                                              in_sw_list,
                                              num_software,
                                              byref(results),
                                              byref(num_results),
                                              c_int(allow_insecure))

        if self.internal_is_error(ret):
            return (False, ret, None)

        results_out = []
        for idx in range(0, num_results.value):
            raw_res = results[idx]
            if raw_res.status == LIB_EUST_UNKWOWN:
                continue

            res = (software_list[idx].name.decode('ASCII'),
                   self.Result(self.lib_to_usr_status(raw_res.status),
                               self.Version(raw_res.version.major,
                                            raw_res.version.minor,
                                            raw_res.version.revision
                                            .decode('ASCII')),
                               raw_res.link.decode('UTF-8'))
                   )
            results_out.append(res)

        self.lib_obj.updater_free_result_list(results, num_results)

        return (True, ret, results_out)

    def error_to_str(self, err):
        """Translates ECHMETUpdateCheck return code to string representation.

        Args:
            err (int): ECHMETUpdateCheck return code.

        Returns:
            String representation of the return code
        """

        return self.lib_obj.updater_error_to_str(err).decode('ASCII')

    def status_to_str(self, stat):
        """Translates ECHMETUpdateCheck update status to string representation.

        Args:
            stat (int): ECHMETUpdateCheck update status code.

        Returns:
            String representation of the update status
        """

        return self.lib_obj.updater_status_to_str(stat).decode('ASCII')
