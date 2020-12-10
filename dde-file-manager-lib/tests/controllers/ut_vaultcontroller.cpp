#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>


#define private public

#include <QSharedPointer>
#include <dfmevent.h>
#include <controllers/vaultcontroller.h>
#include <interfaces/dfmstandardpaths.h>
#include <io/dfilestatisticsjob.h>
#include <QTimer>
#include "shutil/fileutils.h"

#include "controllers/vaultcontroller.h"
#include "vault/vaultglobaldefine.h"
#include "controllers/vaulterrorcode.h"
#include <QDirIterator>
#include "dfmstandardpaths.h"
#include <interfaces/dfileservices.h>
#include "dfmevent.h"
#include "models/vaultfileinfo.h"
#include <QProcess>

#include "stub.h"
#include "../stub-ext/stubext.h"


DFM_USE_NAMESPACE
namespace  {
    class TestVaultController : public testing::Test
    {
    public:
        QSharedPointer<VaultController> m_controller;

        virtual void SetUp() override
        {
            QString home = DFMStandardPaths::location(DFMStandardPaths::HomePath);
            m_controller = QSharedPointer<VaultController>(new VaultController());
            std::cout << "start TestVaultController" << std::endl;
        }

        virtual void TearDown() override
        {
            std::cout << "end TestVaultController" << std::endl;
        }

        void loop()
        {
            QEventLoop loop;
            QTimer::singleShot(100, nullptr, [&loop]() {
                loop.exit();
            });
            loop.exec();
        }
    };
}


TEST_F(TestVaultController, tst_createFileInfo)
{
#if 0 // those code will cause broen occasionally
    DUrl fileUrl = DUrl::fromVaultFile("/");
    const QSharedPointer<DFMCreateFileInfoEvent> &&event =
            dMakeEventPointer<DFMCreateFileInfoEvent>(nullptr, fileUrl);
    const auto &&pt = m_controller->createFileInfo(event);

    EXPECT_NE(nullptr, pt.data());
#endif
}

TEST_F(TestVaultController, tst_createDirIterator)
{
    QStringList nameFilters;
    nameFilters << "filter";


    const DDirIteratorPointer &iterator = m_controller->createDirIterator(
                dMakeEventPointer<DFMCreateDiriterator>(nullptr,
                                                        DUrl(DFMStandardPaths::location(DFMStandardPaths::HomePath)),
                                                        nameFilters,
                                                        QDir::NoFilter,
                                                        QDirIterator::NoIteratorFlags));
    iterator->next();
    iterator->hasNext();
    iterator->fileName();
    iterator->fileUrl();
    iterator->fileInfo();
    iterator->url();
}

TEST_F(TestVaultController, tst_createFileWatcher)
{
    QSharedPointer<DFMCreateFileWatcherEvent> event = dMakeEventPointer<DFMCreateFileWatcherEvent>(nullptr, DUrl::fromVaultFile("/"));
    auto ret = m_controller->createFileWatcher(event);
    EXPECT_NE(nullptr, ret);
}

TEST_F(TestVaultController, tst_open_file_files)
{
    QSharedPointer<DFMOpenFileEvent> event = dMakeEventPointer<DFMOpenFileEvent>(nullptr, DUrl::fromVaultFile("/"));

    bool (*st_openFile)(const QObject *, const DUrl &) = [](const QObject *, const DUrl &){
        return true;
    };
    Stub stub;
    stub.set(ADDR(DFileService, openFile), st_openFile);
    EXPECT_TRUE(m_controller->openFile(event));

    DUrlList urls;
    urls << DUrl::fromVaultFile("/");
    QSharedPointer<DFMOpenFilesEvent> events = dMakeEventPointer<DFMOpenFilesEvent>(nullptr, urls);

    // replace openFiles to avoid dialogs pop
    bool (*st_openFiles)(const QStringList &) = [](const QStringList &) {
        return true;
    };
    stub.set(ADDR(FileUtils, openFiles), st_openFiles);

    // replace isExecutableScript
    bool (*st_isExecutableScript)(const QString &) = [](const QString &) {
        return true;
    };
    stub.set(ADDR(FileUtils, isExecutableScript), st_isExecutableScript);

    EXPECT_FALSE(m_controller->openFiles(events));

    // repalce isFileRunnable
    stub.reset(ADDR(FileUtils, isExecutableScript));
    bool (*st_isFileRunnable)(const QString &) = [](const QString &) {
        return true;
    };

    stub.set(ADDR(FileUtils, isFileRunnable), st_isFileRunnable);
    EXPECT_FALSE(m_controller->openFiles(events));

    // replace shouldAskUserToAddExecutableFlag
    stub.reset(ADDR(FileUtils, isFileRunnable));
    bool (*st_shouldAskUserToAddExecutableFlag)(const QString &) = [](const QString &) {
        return true;
    };

    stub.set(ADDR(FileUtils, shouldAskUserToAddExecutableFlag), st_shouldAskUserToAddExecutableFlag);
    EXPECT_FALSE(m_controller->openFiles(events));

    // replace isFileWindowUrlShortcut
    stub.reset(ADDR(FileUtils, shouldAskUserToAddExecutableFlag));
    bool (*st_isFileWindowsUrlShortcut)(const QString &) = [](const QString &) {
        return true;
    };
    stub.set(ADDR(FileUtils, isFileWindowsUrlShortcut), st_isFileWindowsUrlShortcut);

    EXPECT_TRUE(m_controller->openFiles(events));
}

/// --------------------------------------------------
///                  static test
/// --------------------------------------------------
TEST_F(TestVaultController, tst_makeVaultUrl)
{
    DUrl url = m_controller->makeVaultUrl("", "file");

    EXPECT_EQ(DFMVAULT_SCHEME, url.scheme());
    EXPECT_EQ("file", url.host());
}

TEST_F(TestVaultController, tst_localUrlToVault)
{
    DUrl url = m_controller->localUrlToVault(DUrl::fromVaultFile("/test/path"));

    EXPECT_FALSE(url.isValid());

    url = m_controller->localUrlToVault(VaultController::makeVaultUrl(VaultController::makeVaultLocalPath()));
    EXPECT_TRUE(url.isValid());

    EXPECT_EQ(DFMVAULT_SCHEME, url.scheme());
    EXPECT_EQ("", url.host());
}

TEST_F(TestVaultController, tst_localToVault)
{
    DUrl url = m_controller->localToVault("/test/path");

    EXPECT_FALSE(url.isValid());

    url = m_controller->localToVault(VaultController::makeVaultLocalPath());
    EXPECT_TRUE(url.isValid());

    EXPECT_EQ(DFMVAULT_SCHEME, url.scheme());
    EXPECT_EQ("", url.host());
}

TEST_F(TestVaultController, tst_vaultToLocal)
{
    QString url = m_controller->vaultToLocal(VaultController::makeVaultUrl());
    EXPECT_FALSE(url.isEmpty());

    DUrl videoUrl = DUrl::fromComputerFile("Videos");
    url = m_controller->vaultToLocal(videoUrl);
    EXPECT_EQ(videoUrl.toLocalFile(), url);
}

TEST_F(TestVaultController, tst_vaultToLocalUrl)
{
    DUrl url = m_controller->vaultToLocalUrl(VaultController::makeVaultUrl());
    EXPECT_TRUE(url.isValid());
}

TEST_F(TestVaultController, tst_vaultToLocalUrls)
{
    DUrlList urls;
    urls << m_controller->makeVaultUrl();
    DUrlList ret = m_controller->vaultToLocalUrls(urls);
    EXPECT_EQ(1, ret.size());
}

TEST_F(TestVaultController, tst_pathToVirtualPath)
{
    QString virPath = m_controller->pathToVirtualPath(VaultController::makeVaultLocalPath());
    QString head = virPath.left(8);
    EXPECT_EQ(DFMVAULT_SCHEME, head);
}

TEST_F(TestVaultController, tst_urlToVirtualUrl)
{
    QString virPath = m_controller->urlToVirtualUrl(VaultController::makeVaultLocalPath()).toString();
    QString head = virPath.left(8);
    EXPECT_EQ(DFMVAULT_SCHEME, head);
}

TEST_F(TestVaultController, tst_isBigFileDeleting)
{
    EXPECT_EQ(m_controller->m_isBigFileDeleting, m_controller->isBigFileDeleting());
}

TEST_F(TestVaultController, tst_isVaultFile)
{
    EXPECT_TRUE(m_controller->isVaultFile(m_controller->makeVaultLocalPath()));
    EXPECT_FALSE(m_controller->isVaultFile(DUrl::fromComputerFile("Videos").toString()));
}

TEST_F(TestVaultController, tst_getPermissions)
{
    QString home = DFMStandardPaths::location(DFMStandardPaths::HomePath);
    QFileDevice::Permissions permissions = m_controller->getPermissions(home);
    EXPECT_TRUE(permissions > 0);
}

TEST_F(TestVaultController, tst_state)
{
    VaultController::VaultState state = m_controller->state();
    QString cryfsBinary = QStandardPaths::findExecutable("cryfs");
    if (cryfsBinary.isEmpty()) {
        EXPECT_EQ(VaultController::NotAvailable, state);
    }
}

TEST_F(TestVaultController, tst_isRootDirectory)
{
    EXPECT_TRUE(m_controller->isRootDirectory(m_controller->makeVaultUrl().toString()));
    EXPECT_FALSE(m_controller->isRootDirectory(DUrl::fromComputerFile("Videos").toString()));
}

TEST_F(TestVaultController, tst_getErrorInfo)
{
    for (int i = 10; i < 34; i++) {
        QString err = m_controller->getErrorInfo(i);
        EXPECT_FALSE(err.isEmpty());
    }
}

TEST_F(TestVaultController, tst_toInternalPath)
{
    QString internalPath = m_controller->toInternalPath(m_controller->makeVaultUrl().toString());
    EXPECT_EQ(DFMVAULT_SCHEME, internalPath.left(8));
    EXPECT_TRUE(internalPath.contains("vault_unlocked"));
}

TEST_F(TestVaultController, tst_toExternalPath)
{
    QString externalPath = m_controller->toExternalPath(
                m_controller->makeVaultUrl(m_controller->makeVaultLocalPath()).toString());
    EXPECT_EQ(DFMVAULT_SCHEME, externalPath.left(8));
    EXPECT_FALSE(externalPath.contains("vault_unlocked"));
}

TEST_F(TestVaultController, tst_totalsize)
{
    EXPECT_EQ(m_controller->m_totalSize, m_controller->totalsize());
}

TEST_F(TestVaultController, tst_set_get_vaultState)
{
    m_controller->setVaultState(VaultController::Encrypted);
    EXPECT_EQ(VaultController::Encrypted, m_controller->getVaultState());
    EXPECT_EQ(m_controller->m_enVaultState, m_controller->getVaultState());
}

TEST_F(TestVaultController, tst_setBigFileIsDeleting)
{
    m_controller->setBigFileIsDeleting(true);
    EXPECT_TRUE(m_controller->m_isBigFileDeleting);

    m_controller->setBigFileIsDeleting(false);
    EXPECT_FALSE(m_controller->m_isBigFileDeleting);
}

TEST_F(TestVaultController, tst_updateFolderSizeLabel)
{
    m_controller->updateFolderSizeLabel(10);
    EXPECT_EQ(10, m_controller->m_totalSize);
}

TEST_F(TestVaultController, tst_vaultLockPath)
{
    EXPECT_TRUE(m_controller->vaultLockPath().contains(VAULT_ENCRYPY_DIR_NAME));
}

TEST_F(TestVaultController, tst_vaultUnlockPath)
{
    EXPECT_TRUE(m_controller->vaultUnlockPath().contains(VAULT_DECRYPT_DIR_NAME));
}

TEST_F(TestVaultController, tst_slotCreateVault)
{
    m_controller->slotCreateVault(static_cast<int>(ErrorCode::Success));
    EXPECT_EQ(VaultController::Unlocked, m_controller->m_enVaultState);
}

TEST_F(TestVaultController, tst_slotUnlockVault)
{
    m_controller->slotCreateVault(static_cast<int>(ErrorCode::Success));
    EXPECT_EQ(VaultController::Unlocked, m_controller->m_enVaultState);
}

TEST_F(TestVaultController, tst_slotLockVault)
{
    m_controller->slotLockVault(static_cast<int>(ErrorCode::Success));
    EXPECT_EQ(VaultController::Encrypted, m_controller->m_enVaultState);
}

TEST_F(TestVaultController, tst_openFileByApp)
{
    QString appName("deepin-editor");
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";
    QString testLnFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utLink";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdLink = QString("ln -s ") + testFile + " " + testLnFile;
    QString cmdRm = QString("rm ") + testFile + " " + testLnFile;

    QProcess::execute(cmdTouch);
    QProcess::execute(cmdLink);
    QSharedPointer<DFMOpenFileByAppEvent> event = dMakeEventPointer<DFMOpenFileByAppEvent>(nullptr, appName, DUrl(testLnFile));


    bool (*st_isSymLink)() = [](){
        return true;
    };
    stub_ext::StubExt stub;
    stub.set(VADDR(VaultFileInfo, isSymLink), st_isSymLink);

    EXPECT_FALSE(m_controller->openFileByApp(event));
    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_deleteFiles)
{
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdRm = QString("rm ") + testFile;

    QProcess::execute(cmdTouch);
    DUrlList urls;
    urls << DUrl(testFile);
    QSharedPointer<DFMDeleteEvent> event = dMakeEventPointer<DFMDeleteEvent>(nullptr, urls);

    EXPECT_TRUE(m_controller->deleteFiles(event));
    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_moveToTrash)
{
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdRm = QString("rm ") + testFile;

    QProcess::execute(cmdTouch);
    DUrlList urls;
    urls << DUrl(testFile);
    QSharedPointer<DFMMoveToTrashEvent> event = dMakeEventPointer<DFMMoveToTrashEvent>(nullptr, urls, false);

    m_controller->moveToTrash(event);
    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_pasteFile)
{
    DUrl picPath(DFMStandardPaths::location(DFMStandardPaths::PicturesPath));
    QSharedPointer<DFMPasteEvent> event = dMakeEventPointer<DFMPasteEvent>(nullptr, DFMGlobal::CopyAction, picPath, DUrlList() << picPath);
    m_controller->pasteFile(event);
}

TEST_F(TestVaultController, tst_writeFilsToClipboard)
{
    DUrl picPath(DFMStandardPaths::location(DFMStandardPaths::PicturesPath));
    QSharedPointer<DFMWriteUrlsToClipboardEvent> event = dMakeEventPointer<DFMWriteUrlsToClipboardEvent>(nullptr, DFMGlobal::CopyAction, DUrlList() << picPath);
    m_controller->writeFilesToClipboard(event);
}

TEST_F(TestVaultController, tst_renameFile)
{
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";
    QString testRenameFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utRename";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdRm = QString("rm ") + testFile + " " + testRenameFile;

    QProcess::execute(cmdTouch);

    QSharedPointer<DFMRenameEvent> event = dMakeEventPointer<DFMRenameEvent>(nullptr, testFile, testRenameFile);
    m_controller->renameFile(event);

    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_share_unshare_Folder)
{
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdRm = QString("rm ") + testFile;

    QProcess::execute(cmdTouch);

    QSharedPointer<DFMFileShareEvent> event = dMakeEventPointer<DFMFileShareEvent>(nullptr, testFile, "utFile");
    EXPECT_FALSE(m_controller->shareFolder(event));

    QSharedPointer<DFMCancelFileShareEvent> cancelEvent = dMakeEventPointer<DFMCancelFileShareEvent>(nullptr, testFile);
    EXPECT_TRUE(m_controller->unShareFolder(cancelEvent));

    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_openTerminal)
{
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdRm = QString("rm ") + testFile;

    QProcess::execute(cmdTouch);

    bool (*st_startDetached)(void *, const QString &) = [](void *, const QString &){
        return true;
    };

    typedef bool(*FunPtr)(const QString &);
    Stub stub;
    stub.set(FunPtr(&QProcess::startDetached), st_startDetached);

    QSharedPointer<DFMOpenInTerminalEvent> event = dMakeEventPointer<DFMOpenInTerminalEvent>(nullptr, testFile);
    EXPECT_TRUE(m_controller->openInTerminal(event));

    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_add_remove_Bookmark)
{
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdRm = QString("rm ") + testFile;
    testFile = "file://" + testFile;

    QProcess::execute(cmdTouch);

    QSharedPointer<DFMAddToBookmarkEvent> eventAdd = dMakeEventPointer<DFMAddToBookmarkEvent>(nullptr, testFile);
    EXPECT_FALSE(m_controller->addToBookmark(eventAdd));

    QSharedPointer<DFMRemoveBookmarkEvent> eventRemove = dMakeEventPointer<DFMRemoveBookmarkEvent>(nullptr, testFile);
    EXPECT_FALSE(m_controller->removeBookmark(eventRemove));

    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_createSymlink)
{
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";
    QString testLnFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utLink";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdLink = QString("ln -s ") + testFile + " " + testLnFile;
    QString cmdRm = QString("rm ") + testFile + " " + testLnFile;

    QProcess::execute(cmdTouch);


    QSharedPointer<DFMCreateSymlinkEvent> event = dMakeEventPointer<DFMCreateSymlinkEvent>(nullptr, testFile, testLnFile);
    EXPECT_FALSE(m_controller->createSymlink(event));

    QString vaultTestFile = "file://" + testFile;
    QString vaultTestLnFile = "file://" + testLnFile;
    QSharedPointer<DFMCreateSymlinkEvent> event2 = dMakeEventPointer<DFMCreateSymlinkEvent>(nullptr, vaultTestFile, vaultTestLnFile);
    EXPECT_TRUE(m_controller->createSymlink(event2));

    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_set_remove_FileTag)
{
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdRm = QString("rm ") + testFile;

    QProcess::execute(cmdTouch);

    QList<QString> qstring;
    QSharedPointer<DFMSetFileTagsEvent> event = dMakeEventPointer<DFMSetFileTagsEvent>(nullptr, testFile, qstring);
    m_controller->setFileTags(event);

    QList<DUrl> qUrls;
    QSharedPointer<DFMGetTagsThroughFilesEvent> event2 = dMakeEventPointer<DFMGetTagsThroughFilesEvent>(nullptr, qUrls);
    m_controller->getTagsThroughFiles(event2);

    QSharedPointer<DFMRemoveTagsOfFileEvent> event3 = dMakeEventPointer<DFMRemoveTagsOfFileEvent>(nullptr, testFile, qstring);
    m_controller->removeTagsOfFile(event3);

    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_updateFileInfo)
{
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";

    QString cmdTouch = QString("touch ") + testFile;
    QString cmdRm = QString("rm ") + testFile;

    DUrlList urls;
    urls << DUrl(testFile);

    // file not exist
    m_controller->updateFileInfo(urls);
    // create file
    QProcess::execute(cmdTouch);
    // insert to map
    m_controller->updateFileInfo(urls);
    // use map info
    m_controller->updateFileInfo(urls);

    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_getFileInfo)
{
    VaultController::FileBaseInfo info;
    info.isExist = false;
    QString testFile = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utFile";
    m_controller->m_mapVaultFileInfo.insert(DUrl(testFile), info);
    VaultController::FileBaseInfo ret = m_controller->getFileInfo(DUrl(testFile));
    EXPECT_FALSE(ret.isExist);
}

TEST_F(TestVaultController, tst_createVault)
{
    QString utDir1 = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utDir1";
    QString utDir2 = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utDir2";

    QString cmdTouch = QString("mkdir ") + utDir1 + " " + utDir2;
    QString cmdRm = QString("rm -r ") + utDir1 + " " + utDir2;

    void (*st_sigCreateVault)(QString, QString, QString) = [](QString, QString, QString){
        // do nothing
    };
    Stub stub;
    stub.set(ADDR(VaultController, sigCreateVault), st_sigCreateVault);

    m_controller->createVault("password", utDir1, utDir2);
    m_controller->createVault("password", utDir1, utDir2);

    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_unLockVault)
{
    QString utDir1 = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utDir1";
    QString utDir2 = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utDir2";

    QString cmdRm = QString("rm -r ") + utDir1 + " " + utDir2;

    m_controller->unlockVault("password", utDir1, utDir2);
    m_controller->unlockVault("password", utDir1, utDir2);

    QProcess::execute(cmdRm);
}

TEST_F(TestVaultController, tst_lockVault)
{
    QString utDir1 = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utDir1";
    QString utDir2 = DFMStandardPaths::location(DFMStandardPaths::PicturesPath) + "/utDir2";

    QString cmdRm = QString("rm -r ") + utDir1 + " " + utDir2;

    m_controller->lockVault();
    m_controller->lockVault(utDir1, utDir2);
}
