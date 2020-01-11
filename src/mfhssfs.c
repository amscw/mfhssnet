#include "mfhssnet.h"
#include "mfhssfs.h"

//-------------------------------------------------------------------------------------------------
// Types
//-------------------------------------------------------------------------------------------------
// файлы регистров
struct reg_attribute {
	struct attribute default_attribute;
	u32 address;
	u32 value;
	char name[32];	// TODO: хорошо бы динамически, чтобы не расходовать память зря
} __attribute__((__packed__));

// каталоги регистров
struct reg_dir {
	struct kobject kobj;
	struct mfhss_priv_ *priv;	// set by probe() function in run-time
};

//-------------------------------------------------------------------------------------------------
// MACRO
//-------------------------------------------------------------------------------------------------
// hardcoded registers
#define REG_DMA_CR_NAME			"cr"
#define REG_DMA_CR_ADDRESS		0x0004
#define REG_DMA_SR_NAME			"sr"
#define REG_DMA_SR_ADDRESS		0x0008
#define REG_DMA_IR_NAME			"ir"
#define REG_DMA_IR_ADDRESS		0x000C
#define REG_DMA_SA_NAME			"sa"
#define REG_DMA_SA_ADDRESS		0x0010
#define REG_DMA_DA_NAME			"da"
#define REG_DMA_DA_ADDRESS		0x0014
#define REG_DMA_SL_NAME			"sl"
#define REG_DMA_SL_ADDRESS		0x0018
#define REG_DMA_DL_NAME			"dl"
#define REG_DMA_DL_ADDRESS		0x001C
#define REG_MLIP_SR_NAME		"sr"
#define REG_MLIP_SR_ADDRESS		0x0020
#define REG_MLIP_IR_NAME		"ir"
#define REG_MLIP_IR_ADDRESS		0x0024
#define REG_MLIP_RST_NAME		"rst"
#define REG_MLIP_RST_ADDRESS	0x0028
#define REG_MLIP_CE_NAME		"ce"
#define REG_MLIP_CE_ADDRESS		0x002C

// имя переменной-регистра (атрибута)
#define REG(dir, reg) reg_##dir##_##reg

// имя переменной-директории
#define DIR_TYPE(dir) dir##_type

// создает регистр (атрибут sysfs)
#define MAKE_REG(dir, reg) \
	static struct reg_attribute REG(dir, reg) = {\
		{\
			.name = REG_##dir##_##reg##_NAME,\
			.mode = S_IRUGO | S_IWUSR\
		}, REG_##dir##_##reg##_ADDRESS, 0\
}

// создает объект с операциями над регистром (sysfs_ops)
#define MAKE_DIR_OPS(dir) \
	static void release_##dir(struct kobject *kobj)\
	{\
		struct reg_dir *dir = container_of(kobj, struct reg_dir, kobj);\
		PDEBUG("destroying object: %s\n", kobj->name);\
		kfree(dir);\
	}\
	\
	static ssize_t sysfs_show_##dir(struct kobject *kobj, struct attribute *attr, char *buf)\
	{\
		unsigned long flags = 0;\
		struct reg_dir *dir = container_of(kobj, struct reg_dir, kobj);\
		struct reg_attribute *a = container_of(attr, struct reg_attribute, default_attribute);\
		spin_lock_irqsave(&dir->priv->lock, flags);\
		a->value = ioread32((void __iomem*)(dir->priv->io_base + a->address));\
		spin_unlock_irqrestore(&dir->priv->lock, flags);\
		PDEBUG("read from %s@0x%X = 0x%X\n", attr->name, a->address, a->value);\
		return scnprintf(buf, PAGE_SIZE, "%d\n", a->value);\
	}\
	\
	static ssize_t sysfs_store_##dir(struct kobject *kobj, struct attribute* attr, const char *buf, size_t len)\
	{\
		unsigned long flags = 0;\
		struct reg_dir *dir = container_of(kobj, struct reg_dir, kobj);\
		struct reg_attribute *a = container_of(attr, struct reg_attribute, default_attribute);\
		sscanf(buf, "%d", &a->value);\
		spin_lock_irqsave(&dir->priv->lock, flags);\
		iowrite32(a->value, (void __iomem*)(dir->priv->io_base + a->address));\
		spin_unlock_irqrestore(&dir->priv->lock, flags);\
		PDEBUG("write 0x%X to %s@0x%X\n", a->value, a->default_attribute.name, a->address);\
		return len;\
	}\
	\
	static struct sysfs_ops dir##_ops = {\
		.show = sysfs_show_##dir,\
		.store = sysfs_store_##dir,\
	};\

// создает тип регистра (тип kobject)
#define MAKE_DIR_TYPE(dir) \
	MAKE_DIR_OPS(dir);\
	struct kobj_type DIR_TYPE(dir) = {\
		.sysfs_ops = &dir##_ops,\
		.default_attrs = dir##_attributes,\
		.release = release_##dir\
	};


//-------------------------------------------------------------------------------------------------
// Prototypes
//-------------------------------------------------------------------------------------------------
static void destroy_dir(struct kobject *kobj);
static ssize_t sysfs_show_reg(struct kobject *kobj, struct attribute *attr, char *buf);
static ssize_t sysfs_store_reg(struct kobject *kobj, struct attribute* attr, const char *buf, size_t len);

//-------------------------------------------------------------------------------------------------
// Variables
//-------------------------------------------------------------------------------------------------
// hardcoded DMA registers
MAKE_REG(DMA, CR);
MAKE_REG(DMA, SR);
MAKE_REG(DMA, IR);
MAKE_REG(DMA, SA);
MAKE_REG(DMA, DA);
MAKE_REG(DMA, SL);
MAKE_REG(DMA, DL);
static struct attribute *DMA_attributes[] = {
	&REG(DMA, CR).default_attribute,
	&REG(DMA, SR).default_attribute,
	&REG(DMA, IR).default_attribute,
	&REG(DMA, SA).default_attribute,
	&REG(DMA, DA).default_attribute,
	&REG(DMA, SL).default_attribute,
	&REG(DMA, DL).default_attribute,
	NULL
};
MAKE_DIR_TYPE(DMA);

// hardcoded MLIP registers
MAKE_REG(MLIP, SR);
MAKE_REG(MLIP, IR);
MAKE_REG(MLIP, RST);
MAKE_REG(MLIP, CE);
static struct attribute *MLIP_attributes[] = {
	&REG(MLIP, SR).default_attribute,
	&REG(MLIP, IR).default_attribute,
	&REG(MLIP, RST).default_attribute,
	&REG(MLIP, CE).default_attribute,
	NULL
};
MAKE_DIR_TYPE(MLIP);

// sysfs objects and attributes for dynamic registers
static struct reg_attribute reg_value = {
	{ .name = "value", .mode = S_IRUGO | S_IWUSR },
	0, 0
};

static struct attribute *default_reg_attrs[] = {
	&reg_value.default_attribute, /* dummy reg */
	NULL
};

static struct sysfs_ops reg_ops = {
	.show = sysfs_show_reg,
	.store = sysfs_store_reg,
};

static struct kobj_type dir_type = {
	.release = destroy_dir,
	.sysfs_ops = &reg_ops,
	.default_attrs = default_reg_attrs,
};

//-------------------------------------------------------------------------------------------------
// Functions
//-------------------------------------------------------------------------------------------------
// FIXME: использовать стандартные механизмы ядра для работы со списками
inline void clean_dir(struct kset *dir)
{
	struct list_head *next, *curr, *head;
	struct kobject *kobj = NULL;

	if (dir == NULL)
		return;

	// PDEBUG("destroying directory: %s\n", dir->kobj.name);

	// если был добавлен хотя бы один объект
	// TODO: вместо этого условия можем в цикле исправить инициализатор: pnext=pcurr
	if (&dir->list != dir->list.next)
	{
		// удаление всех объектов множества
		for (head = &dir->list, curr = head->next, next = 0; next != head;  curr = next)
		{
			next = curr->next;	// запоминаем указатель на следующий объект ДО уничтожения текущего
			kobj = list_entry(curr, struct kobject, entry);
			kobject_del(kobj);
			kobject_put(kobj);
		}
	}
}

/*
 * Static registers support
 */
int create_dma_subdir(struct kset* dir, struct mfhss_priv_ *priv)
{
	int err = 0;
	struct reg_dir *subdir;
	
	if (dir == NULL || priv == NULL)
	{
		PRINT_ERR(err = -EFAULT);
		return err;
	}

	subdir = kzalloc(sizeof *subdir, GFP_KERNEL);
	if (subdir == NULL)
	{
		PRINT_ERR(err = -ENOMEM)
		return err;
	}

	kobject_init(&subdir->kobj, &DIR_TYPE(DMA));
	subdir->kobj.kset = dir;
	err = kobject_add(&subdir->kobj, &dir->kobj, "%s", "dma");
	if (err != 0)
	{
		PRINT_ERR(err);
		kobject_put(&subdir->kobj);
	} else {
		subdir->priv = priv;
	}
	return err;
}

int create_mlip_subdir(struct kset* dir, struct mfhss_priv_ *priv)
{
	int err = 0;
	struct reg_dir *subdir;

	if (dir == NULL || priv == NULL)
	{
		PRINT_ERR(err = -EFAULT);
		return err;
	}

	subdir = kzalloc(sizeof *subdir, GFP_KERNEL);
	if (subdir == NULL)
	{
		PRINT_ERR(err = -ENOMEM)
		return err;
	}

	kobject_init(&subdir->kobj, &DIR_TYPE(MLIP));
	subdir->kobj.kset = dir;
	err = kobject_add(&subdir->kobj, &dir->kobj, "%s", "mlip");
	if (err != 0)
	{
		PRINT_ERR(err);
		kobject_put(&subdir->kobj);
	} else {
		subdir->priv = priv;
	}
	return err;
}

/*
 * Dynamic registers support
 */
static void destroy_dir(struct kobject *kobj)
{
	struct reg_dir *dir = container_of(kobj, struct reg_dir, kobj);
	PDEBUG("destroying object: %s\n", kobj->name);
	kfree(dir);
}

static ssize_t sysfs_show_reg(struct kobject *kobj, struct attribute *attr, char *buf)
{
	unsigned long flags;
	struct reg_dir *dir = container_of(kobj, struct reg_dir, kobj);
	struct reg_attribute *a = container_of(attr, struct reg_attribute, default_attribute);

	spin_lock_irqsave(&dir->priv->lock, flags);
	a->value = ioread32((void __iomem*)(dir->priv->io_base + a->address));
	spin_unlock_irqrestore(&dir->priv->lock, flags);
	PDEBUG("read from %s@0x%X = 0x%X\n", attr->name, a->address, a->value);
	return scnprintf(buf, PAGE_SIZE, "%d\n", a->value);
}

static ssize_t sysfs_store_reg(struct kobject *kobj, struct attribute* attr, const char *buf, size_t len)
{
	unsigned long flags = 0;
	struct reg_dir *dir = container_of(kobj, struct reg_dir, kobj);
	struct reg_attribute *a = container_of(attr, struct reg_attribute, default_attribute);

	sscanf(buf, "%d", &a->value);
	spin_lock_irqsave(&dir->priv->lock, flags);
	iowrite32(a->value, (void __iomem*)(dir->priv->io_base + a->address));
	spin_unlock_irqrestore(&dir->priv->lock, flags);
	PDEBUG("write 0x%X to %s@0x%X\n", a->value, a->default_attribute.name, a->address);
	return len;
}

int create_file(struct kobject *dir, const char *name, unsigned address)
{
	int err = 0;
	struct reg_attribute *a;

	if (dir == NULL || name == NULL)
	{
		PRINT_ERR(err = -EFAULT);
		return err;
	}

	a = kzalloc(sizeof *a, GFP_KERNEL);
	if (a == NULL)
	{
		PRINT_ERR(err = -ENOMEM);
		return err;
	}

	a->address = address;
	strcpy(a->name, name);
	a->default_attribute.name = a->name;
	a->default_attribute.mode = S_IRUGO | S_IWUSR;

	err = sysfs_create_file(dir, &a->default_attribute);
	if (err != 0)
	{
		PRINT_ERR(err);
		kfree(a);
		return err;
	}

	PDEBUG("created file %s in %s/ directory\n", a->name, dir->name);
	return err;
}

int create_dir(struct kset *dir, struct mfhss_priv_ *priv, const char *name)
{
	int err = 0;
	struct reg_dir *subdir;

	if (dir == NULL || name == NULL || priv == NULL)
	{
		PRINT_ERR(err = -EFAULT);
		return err;
	}

	subdir = kzalloc(sizeof *subdir, GFP_KERNEL);
	if (subdir == NULL)
	{
		PRINT_ERR(err = -ENOMEM);
		return err;
	}

	kobject_init(&subdir->kobj, &dir_type);
	subdir->kobj.kset = dir;
	subdir->priv = priv;
	err = kobject_add(&subdir->kobj, &dir->kobj, "%s", name);	// WARNING! name need to be copyed
	if (err != 0)
	{
		PRINT_ERR(err);
		kobject_put(&subdir->kobj);
		return err;
	} 

	PDEBUG("created subdirectory %s/ in %s/ directory\n", name, dir->kobj.name);
	return err;
}
